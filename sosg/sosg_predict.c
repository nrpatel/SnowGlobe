#include "sosg_predict.h"
#include "SDL_net.h"
#include "SDL_gfxPrimitives.h"
#include "SDL_image.h"
#include "SDL_ttf.h"
#include <stdio.h>
#include <string.h>

#define PREDICT_CLIENT_INTERVAL 1000
#define PREDICT_SERVER_NAME "localhost" // TODO: support passing in the address
#define PREDICT_SERVER_PORT 1210
#define PREDICT_SERVER_MTU 1500
#define PREDICT_SERVER_TIMEOUT 5000

#define PREDICT_VISIBLE 0x00FF0066
#define PREDICT_HIDDEN 0xFF000066

typedef struct satellite_struct {
    char *name;
    SDL_Surface *name_surf;
    float longitude;
    float latitude;
    char visibility;
    int x;
    int y;
} sat, *sat_p;

typedef struct sosg_predict_struct {
    char *path;
    SDL_Surface *buffer;
    SDL_Surface *update_surf;
    TTF_Font *font;
    SDL_Thread *client_thread;
    SDL_mutex *update_lock;
    SDL_mutex *client_lock;
    SDL_cond *client_timeout;
    int running;
    int should_update;
    
    // TODO: split the predict client thread into a separate file/struct
    sat *sats;
    int num_sats;
    SDL_Surface *path_surf;
    SDL_Surface *sat_icon;
    SDLNet_SocketSet sockset;
    UDPsocket sock;
    UDPpacket *packet;
    IPaddress server;
} sosg_predict_t;

static int sosg_predict_message(sosg_predict_p predict, char *out, int outlen, char *in, int *inlen)
{
    int received = 0;
    int timeout = PREDICT_SERVER_TIMEOUT;
    
    // copy the outgoing data into the packet
    memcpy(predict->packet->data, out, outlen);
    predict->packet->len = outlen;
    predict->packet->address = predict->server;
    
    int sent = SDLNet_UDP_Send(predict->sock, -1, predict->packet);
    if (!sent) {
        fprintf(stderr, "Error: failed to send %s %s\n", out, SDLNet_GetError());
        return -1;
    }
    
    // quit if we get the packet, recv fails, we time out, or the app is exiting
    while ((received != 1) && (timeout > 0) && predict->running) {
        SDLNet_CheckSockets(predict->sockset, 100);
        received = SDLNet_UDP_Recv(predict->sock, predict->packet);
        timeout -= 100;
    }
    
    if (received != 1) {
        fprintf(stderr, "Error: no response from server %d %d %d\n", received,
            timeout, predict->running);
        return -1;
    }
    
    // copy the received packet into the passed in buffer
    if (predict->packet->len < *inlen) {
        memcpy(in, predict->packet->data, predict->packet->len);
        *inlen = predict->packet->len;
        in[*inlen] = '\0';
    } else {
        fprintf(stderr, "Error: incoming packet %d too large for %d\n", 
            predict->packet->len, *inlen);
        return -1;
    }

    return 0;
}

static int sosg_predict_client_init(sosg_predict_p predict)
{
    predict->sock = SDLNet_UDP_Open(0);
    if (!predict->sock) {
        fprintf(stderr, "Error: Could not open UDP socket %s\n", SDLNet_GetError());
        return -1;
    }
    
    predict->sockset = SDLNet_AllocSocketSet(1);
    if (!predict->sockset) {
        fprintf(stderr, "Error: Could not alloc socket set %s\n", SDLNet_GetError());
        return -1;
    }
    
    if (SDLNet_UDP_AddSocket(predict->sockset, predict->sock) == -1) {
        fprintf(stderr, "Error: Could not add socket to socket set %s\n", SDLNet_GetError());
        return -1;
    }
    
    predict->packet = SDLNet_AllocPacket(PREDICT_SERVER_MTU);
    if (!predict->packet) {
        fprintf(stderr, "Error: Could alloc packet %s\n", SDLNet_GetError());
        return -1;
    }
    
    if (SDLNet_ResolveHost(&predict->server, PREDICT_SERVER_NAME, PREDICT_SERVER_PORT)) {
        fprintf(stderr, "Error: Could not resolve %s:%d %s\n", 
            PREDICT_SERVER_NAME, PREDICT_SERVER_PORT, SDLNet_GetError());
        return -1;
    }
    
    // PREDICT supports a maximum of 24 satellites, so we will too
    predict->sats = calloc(24, sizeof(sat));
    if (!predict->sats) {
        fprintf(stderr, "Error: Could not allocate satellite array\n");
        return -1;
    }
        
    return 0;
}

static int sosg_predict_update_sat(sosg_predict_p predict, sat_p input)
{
    char buf[PREDICT_SERVER_MTU];
    char sendbuf[PREDICT_SERVER_MTU];
    int len = PREDICT_SERVER_MTU;
    int sendlen = 0;

    sendlen = sprintf(sendbuf, "GET_SAT %s\n", input->name);

    if (!sosg_predict_message(predict, sendbuf, sendlen, buf, &len)) {
        // since the first line can have spaces in it, we need to start scanning
        // the string at the second line
        char *values = strchr(buf, '\n');
        int matched = 0;
        if (values) {
            // we only care about three of the values
            matched = sscanf(values, "%f %f %*f %*f %*d %*f %*f %*f %*f %*d %c %*f %*f %*f",
                &input->longitude, &input->latitude, &input->visibility);
        }
        if (!values || matched != 3) {
            fprintf(stderr, "Warning: Malformed update for %s\n", input->name);
            return -1;
        }
    } else {
        fprintf(stderr, "Warning: Failed to update %s\n", input->name);
        return -1;
    }
    
    // convert LonW and LatN to equirectangular pixel coordinates
    input->x = (int)floor((float)(predict->buffer->w - 1)*(540.0-input->longitude)/360.0)%predict->buffer->w;
    input->y = (int)floor((float)(predict->buffer->h - 1)*(90.0-input->latitude)/180.0);
//    printf("%s %f %f %c %d %d\n",input->name, input->longitude, input->latitude,
//        input->visibility, input->x, input->y);
    
    return 0;
}

static int sosg_predict_get_sats(sosg_predict_p predict)
{
    char buf[PREDICT_SERVER_MTU];
    int len = PREDICT_SERVER_MTU;
    int num_sats = 0;
    char *savedptr = NULL;

    if (!sosg_predict_message(predict, "GET_LIST\n", 9, buf, &len)) {
        // each line contains the name of one satellite
        char *sat = strtok_r(buf, "\n", &savedptr);
        while (sat) {
            predict->sats[num_sats++].name = strdup(sat);
            sat = strtok_r(NULL, "\n", &savedptr);
        }
        
        predict->num_sats = num_sats;
    } else {
        fprintf(stderr, "Error: Failed to get satellite list\n");
        return -1;
    }
    
    for (num_sats = 0; num_sats < predict->num_sats; num_sats++) {
        char name[10];
        // clip the name if it is long
        int len = strlen(predict->sats[num_sats].name);
        strncpy(name, predict->sats[num_sats].name, 9);
        if (len > 9) {
            name[7] = '~';
            name[8] = predict->sats[num_sats].name[len-1];
        }
        name[9] = '\0';
        
        // generate a surface of the name
        SDL_Color color = {255, 255, 255};
        predict->sats[num_sats].name_surf = TTF_RenderText_Blended(predict->font,
            name, color);
            
        // get an initial position
        sosg_predict_update_sat(predict, predict->sats + num_sats);
    }
    
    return 0;
}

static int sosg_predict_update_sats(sosg_predict_p predict)
{
    int i = 0;
    SDL_Rect pos;
    
    for (i = 0; i < predict->num_sats; i++) {
        int px = predict->sats[i].x;
        int py = predict->sats[i].y;
        if (!sosg_predict_update_sat(predict, predict->sats + i)) {
            // draw a line segment from the last point to the new one if the
            // satellite moved but didn't wrap around the screen
            if ((px != predict->sats[i].x || py != predict->sats[i].y)
             && (abs(predict->sats[i].x - px) < predict->path_surf->w/4))
                thickLineColor(predict->path_surf, px, py, predict->sats[i].x,
                    predict->sats[i].y, 5,
                    (predict->sats[i].visibility == 'V' ? PREDICT_VISIBLE : PREDICT_HIDDEN));
        }
    }
    
    // lock around blitting to the update_surf since it is used in the main thread
    SDL_mutexP(predict->update_lock);
    SDL_BlitSurface(predict->path_surf, NULL, predict->update_surf, NULL);
    for (i = 0; i < predict->num_sats; i++) {
        // TODO: deal with wrapping around the world
        pos.x = predict->sats[i].x - predict->sat_icon->w/2;
        pos.y = predict->sats[i].y - predict->sat_icon->h/2;
        // highlight visible satellites
        if (predict->sats[i].visibility == 'V') {
            filledCircleColor(predict->update_surf, predict->sats[i].x, 
                predict->sats[i].y, predict->sat_icon->w/2, PREDICT_VISIBLE);
        }
        SDL_BlitSurface(predict->sat_icon, NULL, predict->update_surf, &pos);
        // put the name next to the icon
        pos.x += predict->sat_icon->w;
        SDL_BlitSurface(predict->sats[i].name_surf, NULL, predict->update_surf,
            &pos);
    }
    predict->should_update = 1;
    SDL_mutexV(predict->update_lock);
    
    return 0;
}

static void sosg_predict_client_destroy(sosg_predict_p predict)
{
    if (predict->sock) {
        if (predict->sockset)
            SDLNet_UDP_DelSocket(predict->sockset, predict->sock);
        SDLNet_UDP_Close(predict->sock);
    }
    if (predict->sockset) SDLNet_FreeSocketSet(predict->sockset);
    if (predict->packet) SDLNet_FreePacket(predict->packet);
}

static int sosg_predict_client(void *data)
{
    sosg_predict_p predict = (sosg_predict_p)data;
 
    if (sosg_predict_client_init(predict)) {
        sosg_predict_client_destroy(predict);
        return -1;
    }
 
    if (sosg_predict_get_sats(predict)) {
        sosg_predict_client_destroy(predict);
        return -1;
    }
 
    SDL_mutexP(predict->client_lock);
    while (predict->running) {
        SDL_mutexV(predict->client_lock);
        
        sosg_predict_update_sats(predict);
        
        SDL_mutexP(predict->client_lock);
        // Using cond to sleep between polling the server while still being able
        // to end the thread quickly when destroy is called
        if (SDL_CondWaitTimeout(predict->client_timeout, predict->client_lock, PREDICT_CLIENT_INTERVAL)
                != SDL_MUTEX_TIMEDOUT)
            break;
    }
    SDL_mutexV(predict->client_lock);
 
    sosg_predict_client_destroy(predict);
 
    return 0;   
}

sosg_predict_p sosg_predict_init(const char *path)
{
    sosg_predict_p predict = calloc(1, sizeof(sosg_predict_t));
    if (predict) {
        if (path) predict->path = strdup(path);
        
        predict->update_lock = SDL_CreateMutex();
        predict->client_lock = SDL_CreateMutex();
        predict->client_timeout = SDL_CreateCond();
        
        TTF_Init();
        predict->font = TTF_OpenFont("orbitron-black.otf", 32);
        
        SDL_Surface *surface = IMG_Load(predict->path);
        if (surface) {
            predict->buffer = SDL_CreateRGBSurface(SDL_SWSURFACE, surface->w, 
                surface->h, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
            predict->update_surf = SDL_CreateRGBSurface(SDL_SWSURFACE, surface->w, 
                surface->h, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
            predict->path_surf = SDL_CreateRGBSurface(SDL_SWSURFACE, surface->w, 
                surface->h, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
            SDL_BlitSurface(surface, NULL, predict->buffer, NULL);
            SDL_BlitSurface(surface, NULL, predict->update_surf, NULL);
            SDL_BlitSurface(surface, NULL, predict->path_surf, NULL);
            SDL_FreeSurface(surface);
        } else {
            fprintf(stderr, "Warning: Could not open image at %s\n", predict->path);
        }
        
        predict->sat_icon = IMG_Load("satellite.png");
        if (!predict->sat_icon) {
            fprintf(stderr, "Warning: Could not open satellite.png\n");
        }
        
        predict->running = 1;
        predict->client_thread = SDL_CreateThread(sosg_predict_client, predict);
    }
    
    return predict;
}

void sosg_predict_destroy(sosg_predict_p predict)
{
    int i = 0;

    if (predict) {
        SDL_mutexP(predict->client_lock);
        predict->running = 0;
        SDL_CondSignal(predict->client_timeout);
        SDL_mutexV(predict->client_lock);
        if (predict->client_thread) SDL_WaitThread(predict->client_thread, NULL);
    
        if (predict->path) free(predict->path);
        if (predict->font) TTF_CloseFont(predict->font);
        if (predict->buffer) SDL_FreeSurface(predict->buffer);
        if (predict->update_surf) SDL_FreeSurface(predict->update_surf);
        if (predict->path_surf) SDL_FreeSurface(predict->path_surf);
        if (predict->update_lock) SDL_DestroyMutex(predict->update_lock);
        if (predict->client_lock) SDL_DestroyMutex(predict->client_lock);
        if (predict->client_timeout) SDL_DestroyCond(predict->client_timeout);
        for (i = 0; i < predict->num_sats; i++) {
            if (predict->sats[i].name) free(predict->sats[i].name);
            if (predict->sats[i].name_surf) SDL_FreeSurface(predict->sats[i].name_surf);
        }
        if (predict->sats) free(predict->sats);
        
        free(predict);
        
        TTF_Quit();
    }
}

void sosg_predict_get_resolution(sosg_predict_p predict, int *resolution)
{
    if (resolution && predict && predict->buffer) {
        resolution[0] = predict->buffer->w;
        resolution[1] = predict->buffer->h;
    }
}

SDL_Surface *sosg_predict_update(sosg_predict_p predict)
{
    if (!predict) return NULL;
    
    // We only blit the surface if it was changed by the predict client thread
    SDL_mutexP(predict->update_lock);
    if (predict->should_update) {
        SDL_BlitSurface(predict->update_surf, NULL, predict->buffer, NULL);
        predict->should_update = 0;
    }
    SDL_mutexV(predict->update_lock);
    
    return predict->buffer;
}
