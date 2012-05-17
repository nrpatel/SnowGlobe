/* Based on http://wiki.videolan.org/LibVLC_SampleCode_SDL */

#include "sosg_video.h"
#include <stdio.h>
#include <vlc/vlc.h>

// TODO: Make the size dynamic based on input resolution
#define VIDEOWIDTH 2048
#define VIDEOHEIGHT 1024

typedef struct sosg_video_struct {
    SDL_Surface *buffer;
    SDL_Surface *surface;
    SDL_mutex *mutex;
    libvlc_instance_t *libvlc;
    libvlc_media_list_t *ml;
    libvlc_media_list_player_t *mlp;
    libvlc_media_player_t *mp;
    int num_videos;
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

sosg_video_p sosg_video_init(int num_paths, char *paths[])
{
    sosg_video_p video = calloc(1, sizeof(sosg_video_t));
    if (video) {
        video->mutex = SDL_CreateMutex();
            
        video->buffer = SDL_CreateRGBSurface(SDL_SWSURFACE, VIDEOWIDTH, VIDEOHEIGHT, 32, 
            0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
        video->surface = SDL_CreateRGBSurface(SDL_SWSURFACE, VIDEOWIDTH, VIDEOHEIGHT, 32, 
            0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
        
        char const *vlc_argv[] =
        {
            "--input-repeat=-1",
            //"--no-video-title-show",
            "--no-audio", /* skip any audio track */
            "--no-xlib", /* tell VLC to not use Xlib */
        };        
        int vlc_argc = sizeof(vlc_argv) / sizeof(*vlc_argv);
        
        video->libvlc = libvlc_new(vlc_argc, vlc_argv);
        video->mlp = libvlc_media_list_player_new(video->libvlc);
        video->mp = libvlc_media_player_new(video->libvlc);
        video->ml = libvlc_media_list_new(video->libvlc);
        
        libvlc_media_list_player_set_media_player(video->mlp, video->mp);
        libvlc_media_list_player_set_media_list(video->mlp, video->ml);
        
        int i;
        for (i = 0; i < num_paths; i++) {
            libvlc_media_t *m = libvlc_media_new_path(video->libvlc, paths[i]);
            if (m) {
                video->num_videos++;
                libvlc_media_list_add_media(video->ml, m);
                libvlc_media_release(m);
            }
        }
        
        libvlc_playback_mode_t mode = libvlc_playback_mode_loop;
        libvlc_media_list_player_set_playback_mode(video->mlp, mode);
        
        libvlc_video_set_callbacks(video->mp, lock, unlock, display, video);
        libvlc_video_set_format(video->mp, "RV32", VIDEOWIDTH, VIDEOHEIGHT, VIDEOWIDTH*4);
        
        libvlc_media_list_player_play(video->mlp);
    }

    return video;
}

void sosg_video_destroy(sosg_video_p video)
{
    if (video) {
        if (video->mp) {
            libvlc_media_player_stop(video->mp);
            libvlc_media_player_release(video->mp);
        }
        if (video->ml) libvlc_media_list_release(video->ml);
        if (video->mlp) libvlc_media_list_player_release(video->mlp);
        if (video->libvlc) libvlc_release(video->libvlc);
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

void sosg_video_set_index(sosg_video_p video, int index)
{
    if (video) {
        libvlc_media_list_player_play_item_at_index(video->mlp, index%video->num_videos);
    }
}

SDL_Surface *sosg_video_update(sosg_video_p video)
{
    SDL_LockMutex(video->mutex);
    SDL_BlitSurface(video->buffer, NULL, video->surface, NULL);
    SDL_UnlockMutex(video->mutex);

    return video->surface;
}
