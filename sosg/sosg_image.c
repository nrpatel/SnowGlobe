#include "sosg_image.h"
#include <stdio.h>

typedef struct sosg_image_struct {
    int num_images;
    int index;
    SDL_Surface **buffer;
} sosg_image_t;

sosg_image_p sosg_image_init(int num_paths, char *paths[])
{
    int i;
    sosg_image_p images = calloc(1, sizeof(sosg_image_t));
    if (images) {
        // Allocate space with the assumption that all the paths are valid
        images->buffer = calloc(num_paths, sizeof(SDL_Surface *));
        
        // Load all valid images.
        // TODO: Load dynamically, as this can be a large amount of RAM
        for (i = 0; i < num_paths; i++) {
            SDL_Surface *surface = IMG_Load(paths[i]);
            if (surface) {
                // We blit to a new buffer to ensure the color order and depth are correct
                images->buffer[images->num_images] = SDL_CreateRGBSurface(SDL_SWSURFACE, 
                    surface->w, surface->h, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
                SDL_BlitSurface(surface, NULL, images->buffer[images->num_images], NULL);
                SDL_FreeSurface(surface);
                
                images->num_images++;
            }
        }
        
        images->index = 0;
    }
    
    return images;
}

void sosg_image_destroy(sosg_image_p images)
{
    int i;
    if (images) {
        if (images->buffer) {
            for (i = 0; i < images->num_images; i++)
                SDL_FreeSurface(images->buffer[i]);
            free(images->buffer);
        }
        free(images);
    }
}

void sosg_image_get_resolution(sosg_image_p images, int *resolution)
{
    if (resolution && images && images->num_images) {
        resolution[0] = images->buffer[images->index]->w;
        resolution[1] = images->buffer[images->index]->h;
    }
}

void sosg_image_set_index(sosg_image_p images, int index)
{
    if (images) {
        images->index = index % images->num_images;
    }
}

SDL_Surface *sosg_image_update(sosg_image_p images)
{
    if (!images || !images->num_images) return NULL;
    
    return images->buffer[images->index];
}
