#include "sosg_predict.h"
#include <stdio.h>

typedef struct sosg_predict_struct {
    char *path;
    SDL_Surface *buffer;
} sosg_predict_t;

sosg_predict_p sosg_predict_init(const char *path)
{
    sosg_predict_p predict = calloc(1, sizeof(sosg_predict_t));
    if (predict) {
        if (path) predict->path = strdup(path);
        
        sosg_predict_update(predict);
    }
    
    return predict;
}

void sosg_predict_destroy(sosg_predict_p predict)
{
    if (predict) {
        if (predict->path) free(predict->path);
        if (predict->buffer) SDL_FreeSurface(predict->buffer);
        free(predict);
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
    
    if (!predict->buffer) {
        SDL_Surface *surface = IMG_Load(predict->path);
        if (surface) {
            predict->buffer = SDL_CreateRGBSurface(SDL_SWSURFACE, surface->w, surface->h, 32, 
            0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
            SDL_BlitSurface(surface, NULL, predict->buffer, NULL);
            SDL_FreeSurface(surface);
        }
    } else {
        
    }
    
    return predict->buffer;
}
