#include "sosg_image.h"
#include <stdio.h>

#define PRELOAD_IMAGES 5

typedef struct img_struct {
    char *path;
    SDL_Surface *buffer;
} img_t, *img_p;

typedef struct sosg_image_struct {
    int num_images;
    int num_loaded;
    int index;
    int last_index;
    int updated;
    int running;
    SDL_Thread *load_thread;
    img_p *images;
} sosg_image_t;

static void load_image(img_p img)
{
    SDL_Surface *surface = IMG_Load(img->path);
    if (surface) {
        // We blit to a new buffer to ensure the color order and depth are correct
        img->buffer = SDL_CreateRGBSurface(SDL_SWSURFACE, 
            surface->w, surface->h, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
        SDL_BlitSurface(surface, NULL, img->buffer, NULL);
        SDL_FreeSurface(surface);
    }
}

// TODO: keep the thread running and free images far from the current index
//       to allow loading giant data sets
static int sosg_image_load(void *data)
{
    sosg_image_p images = (sosg_image_p)data;
    int i;
    
    // Load all of the remaining images
    for (i = PRELOAD_IMAGES; i < images->num_images; i++) {
        // Stop early if we're exiting
        if (!images->running) break;
        load_image(images->images[i]);
        images->num_loaded++;
    }
    
    return 0;
}

sosg_image_p sosg_image_init(int num_paths, char *paths[])
{
    int i;
    sosg_image_p images = calloc(1, sizeof(sosg_image_t));
    if (images) {
        // Allocate space with the assumption that all the paths are valid
        images->images = calloc(num_paths, sizeof(img_p));
        images->num_images = num_paths;
        
        // Copy the file paths for each images to load
        // TODO: check if the files actually are valid
        for (i = 0; i < images->num_images; i++) {
            images->images[i] = calloc(1, sizeof(img_t));
            images->images[i]->path = strdup(paths[i]);
        }
        
        // Load in a few images at the start
        int preload = images->num_images > PRELOAD_IMAGES ? PRELOAD_IMAGES : images->num_images;
        for (i = 0; i < preload; i++) {
            load_image(images->images[i]);
            images->num_loaded++;
        }
        
        // If we still have more images left, load them in a new thread
        if (images->num_images > PRELOAD_IMAGES) {
            images->running = 1;
            images->load_thread = SDL_CreateThread(sosg_image_load, images);
        }

        images->index = 0;
        images->updated = 1;
    }
    
    return images;
}

void sosg_image_destroy(sosg_image_p images)
{
    int i;
    if (images) {
        images->running = 0;
        if (images->load_thread) SDL_WaitThread(images->load_thread, NULL);
    
        if (images->images) {
            for (i = 0; i < images->num_images; i++) {
                if (images->images[i]) {
                    if (images->images[i]->buffer) SDL_FreeSurface(images->images[i]->buffer);
                    if (images->images[i]->path) free(images->images[i]->path);
                    free(images->images[i]);
                }
            }
            free(images->images);
        }
        free(images);
    }
}

void sosg_image_get_resolution(sosg_image_p images, int *resolution)
{
    if (resolution && images && images->images[images->index]->buffer) {
        resolution[0] = images->images[images->index]->buffer->w;
        resolution[1] = images->images[images->index]->buffer->h;
    }
}

void sosg_image_set_index(sosg_image_p images, int index)
{
    if (images) {
        // Act on the difference between the last input and the current one
        // to avoid some weirdness while the images are still loading
        int i = index - images->last_index;
        images->last_index = index;
        int new_index = images->index + i;

        // Only use images that have completed loading
        int loaded = images->num_loaded;
        while (new_index < 0) new_index += loaded;
        new_index = new_index % loaded;
        images->index = new_index;
        images->updated = 1;
    }
}

SDL_Surface *sosg_image_update(sosg_image_p images)
{
    // Only pass a surface if we switched to a new image
    if (!images || !images->num_loaded || !images->updated) return NULL;
    
    images->updated = 0;
    return images->images[images->index]->buffer;
}
