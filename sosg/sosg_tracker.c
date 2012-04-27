#include "sosg_tracker.h"
#include "SDL.h"
#include <stdio.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <math.h>

// A NONSTANDARD FOR TRANSMISSION OF IP DATAGRAMS OVER SERIAL LINES: SLIP
// http://tools.ietf.org/rfc/rfc1055.txt
#define END             0xC0    /* indicates end of packet */
#define ESC             0xDB    /* indicates byte stuffing */
#define ESC_END         0xDC    /* ESC ESC_END means END data byte */
#define ESC_ESC         0xDD    /* ESC ESC_ESC means ESC data byte */

enum packet_type {
    PACKET_QUAT = 0,
    PACKET_ACC = 1,
    PACKET_GYRO = 2,
    PACKET_MAG = 3,
    PACKET_COLOR = 4,
    PACKET_BLINK = 5,
    PACKET_MAX = 6
};

typedef struct packet_s {
    unsigned char type;
    union {
        uint32_t net[4];
        float quat[4];
        float sensor[3];
        unsigned char color[4];
    } data;
} packet_t, *packet_p;

#define PACKET_MAX_SIZE (sizeof(unsigned char)+sizeof(float)*4)
#define PACKET_MAX_READ (4096)

typedef struct sosg_tracker_struct {
    int fd;
    SDL_Thread *read_thread;
    int running;
    int mode;
    float rotation;
    float scroll_last;
    float scroll_rotation;
} sosg_tracker_t;

static void tracker_update(sosg_tracker_p tracker, packet_p packet)
{
    // http://en.wikipedia.org/wiki/Conversion_between_quaternions_and_Euler_angles
    float qq2 = packet->data.quat[2]*packet->data.quat[2];
    float roll = atan2(2.0*(packet->data.quat[0]*packet->data.quat[1]
                        +packet->data.quat[2]*packet->data.quat[3]),
                        1.0-2.0*(packet->data.quat[1]*packet->data.quat[1]+qq2));
    float pitch = asin(2.0*(packet->data.quat[0]*packet->data.quat[2]
                        -packet->data.quat[3]*packet->data.quat[1]));
    // use how far the yaw axis is from vertical to switch between the modes
    float mode_angle = sqrt(roll*roll+pitch*pitch);
    float rotation;
    
    if ((mode_angle < M_PI/2.5) || (mode_angle > M_PI-M_PI/2.5)) {
        rotation = atan2(2.0*(packet->data.quat[0]*packet->data.quat[3]
                                +packet->data.quat[1]*packet->data.quat[2]),
                                1.0-2.0*(qq2+packet->data.quat[3]*packet->data.quat[3]));
    } else {
        // Stop using the yaw as we approach gimbal lock
        if (tracker->mode == TRACKER_ROTATE) rotation = tracker->rotation;
        else rotation = tracker->scroll_last;
    }
    
    // Have some hysteresis on switching between modes so we don't jitter at
    // the border
    if ((tracker->mode == TRACKER_ROTATE) && (mode_angle > (0.5*M_PI)/0.95)) {
        tracker->mode = TRACKER_SCROLL;
        tracker->scroll_last = rotation;
    } else if ((tracker->mode == TRACKER_SCROLL) && (mode_angle < (0.5*M_PI)*0.95)) {
        tracker->mode = TRACKER_ROTATE;
    }
    
    if (tracker->mode == TRACKER_ROTATE) {
        tracker->rotation = rotation;
    } else {
        float offset = rotation - tracker->scroll_last;
        tracker->scroll_last = rotation;
        // handle rollover naively
        if (offset < -M_PI) offset += 2.0*M_PI;
        else if (offset > M_PI) offset -= 2.0*M_PI;
        tracker->scroll_rotation += offset;
        tracker->rotation = tracker->scroll_rotation;
    }

//    printf("%f %f %f %d %f\n", roll, pitch, mode_angle, tracker->mode, tracker->rotation);
}

static int tracker_parse(packet_p packet, unsigned char *buf, int len)
{
    int i;
    
    // For now, we only care about reading quaternions from the Tracker
    if ((len == PACKET_MAX_SIZE) && (buf[0] == PACKET_QUAT)) {
        packet->type = PACKET_QUAT;
        for (i = 0; i < 4; i++) {
            // Type punning the uint32_t to a float through the union.  This
            // is not strictly legal in C99, but it is common enough that
            // all compilers seem to allow it.
            packet->data.net[i] = ntohl(*(uint32_t *)(buf+i*4+1));
        }
        
        return 1;
    }
    
    return 0;
}

static int tracker_read(void *data)
{
    sosg_tracker_p tracker = (sosg_tracker_p)data;
    fd_set set;
    struct timeval timeout;
    unsigned char buf[PACKET_MAX_SIZE+1];
    int len = 0;
    int escaping = 0;
    packet_t packet;
    
    FD_ZERO(&set);
    
    while (tracker->running) {
        timeout.tv_sec = 0;
        timeout.tv_usec = 10000; // 10ms
        FD_SET(tracker->fd, &set);
        // read is set to be non-blocking, so wait on the fd with a timeout
        select(tracker->fd+1, &set, NULL, NULL, &timeout);
        // Read available bytes one at a time and unSLIP them into the buffer
        while (1) {
            unsigned char readbuf[PACKET_MAX_READ];
            int i;
            int numread = read(tracker->fd, readbuf, PACKET_MAX_READ);
            if (numread < 1) break; // there was nothing waiting for us
            
            for (i = 0; i < numread; i++) {
                switch (readbuf[i]) {
                    case END:
                        if (tracker_parse(&packet, buf, len)) {
                            tracker_update(tracker, &packet);
                        }
                        len = 0;
                        break;
                    case ESC:
                        escaping = 1;
                        break;
                    case ESC_END:
                        if (escaping) *(buf+len) = END;
                        else *(buf+len) = ESC_END;
                        len++;
                        break;
                    case ESC_ESC:
                        if (escaping) *(buf+len) = ESC;
                        else *(buf+len) = ESC_ESC;
                        len++;
                        break;
                    default:
                        *(buf+len) = readbuf[i];
                        len++;
                        break;
                }
                
                if (readbuf[i] != ESC) escaping = 0;
                // reset the buffer if it no packet was formed
                if (len > PACKET_MAX_SIZE) len = 0;
            }
        }
    }
    
    return 0;
}

sosg_tracker_p sosg_tracker_init(const char *device)
{
    sosg_tracker_p tracker = calloc(1, sizeof(sosg_tracker_t));
    if (tracker) {
        tracker->fd = open(device, O_RDWR | O_NOCTTY | O_NONBLOCK);
        if (tracker->fd == -1) {
            fprintf(stderr, "Error: failed to open Tracker at %s\n", device);
            free(tracker);
            return NULL;
        }
        
        tracker->running = 1;
        tracker->read_thread = SDL_CreateThread(tracker_read, tracker);
    }
    
    return tracker;
}

void sosg_tracker_destroy(sosg_tracker_p tracker)
{
    if (tracker) {
        tracker->running = 0;
        if (tracker->read_thread) SDL_WaitThread(tracker->read_thread, NULL);
        close(tracker->fd);
        
        free(tracker);
    }
}

void sosg_tracker_get_rotation(sosg_tracker_p tracker, float *rotation, int *mode)
{
    if (tracker) {
        if (rotation) *rotation = tracker->rotation;
        if (mode) *mode = tracker->mode;
    }
}

