#ifndef _SOSG_IMAGE_H_
#define _SOSG_IMAGE_H_

#include "SDL.h"
#include "SDL_image.h"

typedef struct sosg_image_struct *sosg_image_p;

sosg_image_p sosg_image_init(const char *path);
void sosg_image_destroy(sosg_image_p images);
void sosg_image_get_resolution(sosg_image_p images, int *resolution);
SDL_Surface *sosg_image_update(sosg_image_p images);

#endif /* _SOSG_IMAGE_H_ */
