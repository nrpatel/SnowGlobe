#ifndef _SOSG_PREDICT_H_
#define _SOSG_PREDICT_H_

#include "SDL.h"

typedef struct sosg_predict_struct *sosg_predict_p;

sosg_predict_p sosg_predict_init(const char *path);
void sosg_predict_destroy(sosg_predict_p predict);
void sosg_predict_get_resolution(sosg_predict_p predict, int *resolution);
SDL_Surface *sosg_predict_update(sosg_predict_p predict);

#endif /* _SOSG_PREDICT_H_ */
