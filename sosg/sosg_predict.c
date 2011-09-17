#include "sosg_predict.h"
#include "SDL_net.h"
#include <stdio.h>

#define PREDICT_CLIENT_INTERVAL 5000
#define PREDICT_SERVER_NAME "localhost"
#define PREDICT_SERVER_PORT 1210
#define PREDICT_SERVER_MTU 1500
#define PREDICT_SERVER_TIMEOUT 5000

typedef struct satellite_struct {
    char *name;
    float longitude;
    float latitude;
    char visibility;
} sat;

typedef struct sosg_predict_struct {
    char *path;
    SDL_Surface *buffer;
    SDL_Surface *update_surf;
    SDL_Surface *path_surf;
    SDL_Thread *client_thread;
    SDL_mutex *update_lock;
    SDL_mutex *client_lock;
    SDL_cond *client_timeout;
    int running;
    int should_update;
    
    sat *sats;
    int num_sats;
    SDLNet_SocketSet sockset;
    UDPsocket sock;
    UDPpacket *packet;
    IPaddress server;
} sosg_predict_t;

static int sosg_predict_message(sosg_predict_p predict, char *out, int outlen, char *in, int *inlen)
{
    int received = 0;
    int timeout = PREDICT_SERVER_TIMEOUT;
    memcpy(predict->packet->data, out, outlen);
    predict->packet->len = outlen;
    predict->packet->address = predict->server;
    
    int sent = SDLNet_UDP_Send(predict->sock, -1, predict->packet);
    if (!sent) {
        fprintf(stderr, "Error: failed to send %s %s\n", out, SDLNet_GetError());
        return -1;
    }
    
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
    
    if (predict->packet->len < *inlen)
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
        return -1;
    }
 
    SDL_mutexP(predict->client_lock);
    while (predict->running) {
        SDL_mutexV(predict->client_lock);
        
        char buf[PREDICT_SERVER_MTU];
        int len = PREDICT_SERVER_MTU;
        if (!sosg_predict_message(predict, "GET_TIME\n", 9, buf, &len))
            printf("got %s\n", buf);
        
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
            fprintf(stderr, "Error: Could not open image at %s\n", predict->path);
        }
        
        predict->running = 1;
        predict->client_thread = SDL_CreateThread(sosg_predict_client, predict);
    }
    
    return predict;
}

void sosg_predict_destroy(sosg_predict_p predict)
{
    if (predict) {
        SDL_mutexP(predict->client_lock);
        predict->running = 0;
        SDL_CondSignal(predict->client_timeout);
        SDL_mutexV(predict->client_lock);
        if (predict->client_thread) SDL_WaitThread(predict->client_thread, NULL);
    
        if (predict->path) free(predict->path);
        if (predict->buffer) SDL_FreeSurface(predict->buffer);
        if (predict->update_surf) SDL_FreeSurface(predict->update_surf);
        if (predict->path_surf) SDL_FreeSurface(predict->path_surf);
        if (predict->update_lock) SDL_DestroyMutex(predict->update_lock);
        if (predict->client_lock) SDL_DestroyMutex(predict->client_lock);
        if (predict->client_timeout) SDL_DestroyCond(predict->client_timeout);
        
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
    
    // We only blit the surface if it was changed by the predict client thread
    SDL_mutexP(predict->update_lock);
    if (predict->should_update) {
        SDL_BlitSurface(predict->update_surf, NULL, predict->buffer, NULL);
        predict->should_update = 0;
    }
    SDL_mutexV(predict->update_lock);
    
    return predict->buffer;
}
