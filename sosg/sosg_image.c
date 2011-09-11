#include "sosg_image.h"
#include <stdio.h>

typedef struct sosg_image_struct {
    char *path;
    int num_images;
    SDL_Surface *buffer;
} sosg_image_t;

sosg_image_p sosg_image_init(const char *path)
{
    sosg_image_p images = calloc(1, sizeof(sosg_image_t));
    if (images) {
        if (path) images->path = strdup(path);
        images->num_images = 1;
        
        sosg_image_update(images);
    }
    
    return images;
}

void sosg_image_destroy(sosg_image_p images)
{
    if (images) {
        if (images->path) free(images->path);
        if (images->buffer) SDL_FreeSurface(images->buffer);
        free(images);
    }
}

void sosg_image_get_resolution(sosg_image_p images, int *resolution)
{
    if (resolution && images && images->buffer) {
        resolution[0] = images->buffer->w;
        resolution[1] = images->buffer->h;
    }
}

SDL_Surface *sosg_image_update(sosg_image_p images)
{
    if (!images) return NULL;
    
    if (!images->buffer) {
        SDL_Surface *surface = IMG_Load(images->path);
        if (surface) {
            images->buffer = SDL_CreateRGBSurface(SDL_SWSURFACE, surface->w, surface->h, 32, 
            0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
            SDL_BlitSurface(surface, NULL, images->buffer, NULL);
            SDL_FreeSurface(surface);
        }
    } else if (images->num_images > 1) {
        // TODO: actually load additional images
        SDL_Surface *surface = IMG_Load(images->path);
        if (surface) {
            SDL_BlitSurface(surface, NULL, images->buffer, NULL);
            SDL_FreeSurface(surface);
        }
    }
    
    return images->buffer;
}
