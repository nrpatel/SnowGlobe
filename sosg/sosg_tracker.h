#ifndef _SOSG_TRACKER_H_
#define _SOSG_TRACKER_H_

enum sosg_tracker_mode {
    TRACKER_SCROLL, // Use the Tracker to switch slideshow images or scrub video
    TRACKER_ROTATE  // Use it to rotate the globe
};

typedef struct sosg_tracker_struct *sosg_tracker_p;

sosg_tracker_p sosg_tracker_init(const char *device);
void sosg_tracker_destroy(sosg_tracker_p tracker);
void sosg_tracker_get_rotation(sosg_tracker_p tracker, float *rotation, int *mode);

#endif /* _SOSG_TRACKER_H_ */
