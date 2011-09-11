#include "sosg_video.h"
#include <stdio.h>
#include <vlc/vlc.h>

#define VIDEOWIDTH 1024
#define VIDEOHEIGHT 512

typedef struct sosg_video_struct {
    char *path;
    SDL_Surface *buffer;
    SDL_Surface *surface;
    SDL_mutex *mutex;
    libvlc_instance_t *libvlc;
    libvlc_media_t *m;
    libvlc_media_player_t *mp;
} sosg_video_t;

static void *lock(void *data, void **p_pixels)
{
    sosg_video_p video = data;

    SDL_LockMutex(video->mutex);
    SDL_LockSurface(video->buffer);
    *p_pixels = video->buffer->pixels;
    return NULL; /* picture identifier, not needed here */
}

static void unlock(void *data, void *id, void *const *p_pixels)
{
    sosg_video_p video = data;

    SDL_UnlockSurface(video->buffer);
    SDL_UnlockMutex(video->mutex);
}

static void display(void *data, void *id)
{
    
}

sosg_video_p sosg_video_init(const char *path)
{
    sosg_video_p video = calloc(1, sizeof(sosg_video_t));
    if (video) {
        if (path) video->path = strdup(path);
        video->mutex = SDL_CreateMutex();
            
        video->buffer = SDL_CreateRGBSurface(SDL_SWSURFACE, VIDEOWIDTH, VIDEOHEIGHT, 32, 
            0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
        video->surface = SDL_CreateRGBSurface(SDL_SWSURFACE, VIDEOWIDTH, VIDEOHEIGHT, 32, 
            0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
        
        char const *vlc_argv[] =
        {
            "--input-repeat=-1",
            "--no-video-title-show",
            "--no-audio", /* skip any audio track */
            "--no-xlib", /* tell VLC to not use Xlib */
        };        
        int vlc_argc = sizeof(vlc_argv) / sizeof(*vlc_argv);
        video->libvlc = libvlc_new(vlc_argc, vlc_argv);
        video->m = libvlc_media_new_path(video->libvlc, video->path);
        video->mp = libvlc_media_player_new_from_media(video->m);
        
        libvlc_video_set_callbacks(video->mp, lock, unlock, display, video);
        libvlc_video_set_format(video->mp, "RV32", VIDEOWIDTH, VIDEOHEIGHT, VIDEOWIDTH*2);
        libvlc_media_player_play(video->mp);
    }

    return video;
}

void sosg_video_destroy(sosg_video_p video)
{
    if (video) {
        if (video->m) libvlc_media_release(video->m);
        if (video->mp) {
            libvlc_media_player_stop(video->mp);
            libvlc_media_player_release(video->mp);
        }
        if (video->libvlc) libvlc_release(video->libvlc);
        if (video->path) free(video->path);
        if (video->buffer) SDL_FreeSurface(video->buffer);
        if (video->mutex) SDL_DestroyMutex(video->mutex);
        free(video);
    }
}

void sosg_video_get_resolution(sosg_video_p video, int *resolution)
{
    if (resolution) {
        resolution[0] = VIDEOWIDTH;
        resolution[1] = VIDEOHEIGHT;
    }
}

SDL_Surface *sosg_video_update(sosg_video_p video)
{
    SDL_LockMutex(video->mutex);
    SDL_BlitSurface(video->buffer, NULL, video->surface, NULL);
    SDL_UnlockMutex(video->mutex);

    return video->surface;
}
