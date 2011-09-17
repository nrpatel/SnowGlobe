/* Science on a Snow Globe
 * by Nirav Patel <nrp@eclecti.cc>
 *
 * An extremely basic take on supporting Science On a Sphere datasets on
 * Snow Globe, a low cost DIY spherical display.
 * Datasets and SOS information available at http://sos.noaa.gov
 * Snow Globe information at http://eclecti.cc
 *
 * Parts are copied from some public domain code from
 * Kyle Foley: http://gpwiki.org/index.php/SDL:Tutorials:Using_SDL_with_OpenGL
 * John Tsiombikas: http://nuclear.mutantstargoat.com/articles/sdr_fract/
 */

#include "SDL.h"
#include "SDL_opengl.h"

#include "sosg_image.h"
#include "sosg_video.h"
#include "sosg_predict.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> // TODO: use the windows equivalent when on windows
#include <math.h>

#define TICK_INTERVAL 33
#define ROTATION_INTERVAL M_PI/(120.0*(1000.0/TICK_INTERVAL))
#define ROTATION_CONSTANT (float)30.5*ROTATION_INTERVAL
#define CLOSE_ENOUGH(a, b) (fabs(a - b) < ROTATION_INTERVAL/2)

enum sosg_mode {
    SOSG_IMAGES,
    SOSG_VIDEO,
    SOSG_PREDICT
};

typedef struct sosg_struct {
    int w;
    int h;
    int fullscreen;
    int texres[2];
    float radius;
    float height;
    float center[2];
    float rotation[2];
    float drotation[2];
    Uint32 time;
    int mode;
    // TODO: use function pointers for different sources
    union {
        sosg_image_p images;
        sosg_video_p video;
        sosg_predict_p predict;
    } source;
    SDL_Surface *screen;
    GLuint texture;
    GLuint program;
    GLuint vertex;
    GLuint fragment;
    GLuint lrotation;
} sosg_t, *sosg_p;

static void load_texture(sosg_p data, SDL_Surface *surface)
{
    // Bind the texture object
    glBindTexture(GL_TEXTURE_2D, data->texture);
    
    // Edit the texture object's image data using the information SDL_Surface gives us
    glTexImage2D(GL_TEXTURE_2D, 0, 4, surface->w, surface->h, 0, 
                  GL_BGRA, GL_UNSIGNED_BYTE, surface->pixels);
}

static char *load_file(char *filename)
{
	char *buf = NULL;
    FILE *fp;
    int len;

	if(!(fp = fopen(filename, "r"))) {
		fprintf(stderr, "failed to open shader: %s\n", filename);
		return NULL;
	}
	
	fseek(fp, 0, SEEK_END);
	len = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	buf = malloc(len + 1);

	len = fread(buf, 1, len, fp);
	buf[len] = '\0';
	fclose(fp);
	
	return buf;
}

static int load_shaders(sosg_p data)
{
    char *vbuf, *fbuf;
    
    vbuf = load_file("sosg.vert");
    if (vbuf) {
        fbuf = load_file("sosg.frag");
        if (!fbuf) {
            free(vbuf);
            return 1;
        }
    } else {
        return 1;
    }
    
    data->vertex = glCreateShader(GL_VERTEX_SHADER);
    data->fragment = glCreateShader(GL_FRAGMENT_SHADER);
    
    glShaderSource(data->vertex, 1, (const GLchar **)&vbuf, NULL);
    glShaderSource(data->fragment, 1, (const GLchar **)&fbuf, NULL);
    
    free(vbuf);
    free(fbuf);
    
    glCompileShader(data->vertex);
    glCompileShader(data->fragment);
    
    data->program = glCreateProgram();
    glAttachShader(data->program, data->vertex);
    glAttachShader(data->program, data->fragment);
    glLinkProgram(data->program);
    glUseProgram(data->program);
    
    // Set the uniforms the fragment shader will need
    GLint loc = glGetUniformLocation(data->program, "radius");
    glUniform1f(loc, data->radius/(float)data->h);
    loc = glGetUniformLocation(data->program, "height");
    glUniform1f(loc, data->height/data->radius);
    loc = glGetUniformLocation(data->program, "center");
    glUniform2f(loc, data->center[0]/(float)data->w, data->center[1]/(float)data->h);
    loc = glGetUniformLocation(data->program, "ratio");
    glUniform1f(loc, (float)data->w/(float)data->h);
    loc = glGetUniformLocation(data->program, "texres");
    glUniform2f(loc, 1.0/(float)data->texres[0], 1.0/(float)data->texres[1]);
    data->lrotation = glGetUniformLocation(data->program, "rotation");
    
    return 0;
}   

static int setup(sosg_p data)
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        printf("Unable to initialize SDL: %s\n", SDL_GetError());
        return 1;
    }
    
    data->time = SDL_GetTicks();
    SDL_EnableKeyRepeat(250, TICK_INTERVAL);
    SDL_ShowCursor(SDL_DISABLE);
    
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    int flags = SDL_OPENGL | (data->fullscreen ? SDL_FULLSCREEN : 0);
    data->screen = SDL_SetVideoMode(data->w, data->h, 32, flags);
    if (!data->screen) {
		printf("Unable to set video mode: %s\n", SDL_GetError());
		SDL_Quit();
		return 1;
	}
	
    // Set the OpenGL state after creating the context with SDL_SetVideoMode
	glClearColor(0, 0, 0, 0);
	glEnable(GL_TEXTURE_2D); // Need this to display a texture
    glViewport(0, 0, data->w, data->h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, data->w, data->h, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    // Have OpenGL generate a texture object handle for us
    glGenTextures(1, &data->texture);
    
    // Bind the texture object
    glBindTexture(GL_TEXTURE_2D, data->texture);
    
    // Set the texture's stretching properties
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    return 0;
}

static void timer_update(sosg_p data)
{
    Uint32 now = SDL_GetTicks();

    if (data->time > now) {
        SDL_Delay(data->time - now);
    }

    while (data->time <= now) {
        data->time += TICK_INTERVAL;
    }
}

static int handle_events(sosg_p data)
{
    SDL_Event event;
    
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym) {
                    case SDLK_ESCAPE:
                        return -1;
                    case SDLK_LEFT:
                        if (event.key.keysym.mod & KMOD_SHIFT)
                            data->drotation[0] += ROTATION_INTERVAL;
                        else
                            data->drotation[0] = ROTATION_CONSTANT;
                        break;
                    case SDLK_RIGHT:
                        if (event.key.keysym.mod & KMOD_SHIFT)
                            data->drotation[0] -= ROTATION_INTERVAL;
                        else
                            data->drotation[0] = -ROTATION_CONSTANT;
                        break;
                    case SDLK_UP:
                        if (event.key.keysym.mod & KMOD_SHIFT)
                            data->drotation[1] += ROTATION_INTERVAL;
                        else
                            data->drotation[1] = ROTATION_CONSTANT;
                        break;
                    case SDLK_DOWN:
                        if (event.key.keysym.mod & KMOD_SHIFT)
                            data->drotation[1] -= ROTATION_INTERVAL;
                        else
                            data->drotation[1] = -ROTATION_CONSTANT;
                        break;
                    case SDLK_p:
                        data->drotation[0] = 0.0;
                        data->drotation[1] = 0.0;
                        break;
                    case SDLK_r:
                        data->rotation[0] = M_PI;
                        data->rotation[1] = 0.0;
                        break;
                    default:
                        break;
                }
                break;
            case SDL_KEYUP:
                // On key up, only if we had ROTATION_CONSTANT going, stop the rotation
                switch (event.key.keysym.sym) {
                    case SDLK_LEFT:
                        if (CLOSE_ENOUGH(data->drotation[0],ROTATION_CONSTANT))
                            data->drotation[0] = 0.0;
                        break;
                    case SDLK_RIGHT:
                        if (CLOSE_ENOUGH(data->drotation[0],-ROTATION_CONSTANT))
                            data->drotation[0] = 0.0;
                        break;
                    case SDLK_UP:
                        if (CLOSE_ENOUGH(data->drotation[1],ROTATION_CONSTANT))
                            data->drotation[1] = 0.0;
                        break;
                    case SDLK_DOWN:
                        if (CLOSE_ENOUGH(data->drotation[1],-ROTATION_CONSTANT))
                            data->drotation[1] = 0.0;
                        break;
                    default:
                        break;
                }
                break;
            case SDL_QUIT:
                return -1;
            default:
                break;
        }
    }
    
    return 0;
}

static void update_media(sosg_p data)
{
    SDL_Surface *surface = NULL;

    switch (data->mode) {
        case SOSG_IMAGES:
            surface = sosg_image_update(data->source.images);
            break;
        case SOSG_VIDEO:
            surface = sosg_video_update(data->source.video);
            break;
        case SOSG_PREDICT:
            surface = sosg_predict_update(data->source.predict);
            break;
    }

    if (surface) {
        // TODO: Support arbitrary resolution images
        // Check that the image's dimensions are a power of 2
        if ((surface->w & (surface->w - 1)) != 0 ||
            (surface->h & (surface->h - 1)) != 0) {
            printf("warning: dimensions (%d, %d) not a power of 2\n",
                surface->w, surface->h);
        }
    
        load_texture(data, surface);
    }
}

static void update_display(sosg_p data)
{
    glUniform2f(data->lrotation, data->rotation[0], data->rotation[1]);

    // Clear the screen before drawing
	glClear(GL_COLOR_BUFFER_BIT);
    
    // Bind the texture to which subsequent calls refer to
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, data->texture);

    // Just make a full screen quad, a canvas for the shader to draw on
    glBegin(GL_QUADS);
        glTexCoord2i(0, 0);
        glVertex3f(0, 0, 0);
    
        glTexCoord2i(1, 0);
        glVertex3f(data->w, 0, 0);
    
        glTexCoord2i(1, 1);
        glVertex3f(data->w, data->h, 0);
    
        glTexCoord2i(0, 1);
        glVertex3f(0, data->h, 0);
    glEnd();
	
    SDL_GL_SwapBuffers();
}

static void usage(sosg_p data)
{
    printf("Usage: sosg [OPTION] [FILE]\n\n");
    printf("sosg is  a simple viewer for NOAA Science on a Sphere datasets\n");
    printf("on Snow Globe, a low cost, open source, DIY spherical display.\n");
    printf("SOS Datasets available at: http://sos.noaa.gov\n");
    printf("Snow Globe information at: http://eclecti.cc\n\n");
    printf("    Input Data\n");
    printf("        -i     Display an image or slideshow (Default)\n");
    printf("        -v     Display a video\n\n");
    printf("        -p     Satellite tracking as a PREDICT client\n");
    printf("    Snow Globe Configuration\n");
    printf("        -f     Fullscreen\n");
    printf("        -w     Display width in pixels (%d)\n", data->w);
    printf("        -h     Display height in pixels (%d)\n", data->h);
    printf("        -r     Radius in pixels (%.1f)\n", data->radius);
    printf("        -x     X offset in pixels (%.1f)\n", data->center[0]);
    printf("        -y     Y offset in pixels (%.1f)\n", data->center[1]);
    printf("        -o     Lens offset in pixels (%.1f)\n\n", data->height);
    printf("The left and right arrow keys can be used to rotate the sphere.\n");
    printf("Holding shift while using the arrows changes rotation speed.\n");
    printf("p will stop the rotation and r resets the angle.\n\n");
}

static void cleanup(sosg_p data)
{
    switch (data->mode) {
        case SOSG_IMAGES:
            sosg_image_destroy(data->source.images);
            break;
        case SOSG_VIDEO:
            sosg_video_destroy(data->source.video);
            break;
        case SOSG_PREDICT:
            sosg_predict_destroy(data->source.predict);
            break;
    }
    
    // Now we can delete the OpenGL texture and close down SDL
    glDeleteTextures(1, &data->texture);
    SDL_Quit();
}

int main(int argc, char *argv[])
{
    int c;
    char *filename = NULL;
    
    sosg_p data = calloc(1, sizeof(sosg_t));
    if (!data) {
        printf("Could not allocate data\n");
        return 1;
    }
    
    // Defaults are for my Snow Globe (not the only Snow Globe anymore!)
    data->w = 848;
    data->h = 480;
    data->radius = 378.0;
    data->height = 370.0;
    data->center[0] = 431.0;
    data->center[1] = 210.0;
    data->rotation[0] = M_PI;
    data->rotation[1] = 0.0;
    
    while ((c = getopt(argc, argv, "ivpfw:g:r:x:y:o:")) != -1) {
        switch (c) {
            case 'i':
                data->mode = SOSG_IMAGES;
                break;
            case 'v':
                data->mode = SOSG_VIDEO;
                break;
            case 'p':
                data->mode = SOSG_PREDICT;
                break;
            case 'f':
                data->fullscreen = 1;
                break;
            case 'w':
                data->w = atoi(optarg);
                break;
            case 'h':
                data->h = atoi(optarg);
                break;
            case 'r':
                data->radius = atof(optarg);
                break;
            case 'x':
                data->center[0] = atof(optarg);
                break;
            case 'y':
                data->center[1] = atof(optarg);
                break;
            case 'o':
                data->height = atof(optarg);
                break;
            case '?':
            default:
                usage(data);
                fprintf(stderr, "Error: failed at option %c\n", optopt);
                return 1;
        }
    }
    
    // Pick the last non-option arg as the filename to use
    for (c = optind; c < argc; c++)
        filename = argv[c];
    
    if (!filename) {
        usage(data);
        fprintf(stderr, "Error: Missing filename or path.\n");
        return 1;
    }

    if (setup(data)) {
        cleanup(data);
        return 1;
    }
    
    switch (data->mode) {
        case SOSG_IMAGES:
            data->source.images = sosg_image_init(filename);
            sosg_image_get_resolution(data->source.images, data->texres);
            break;
        case SOSG_VIDEO:
            data->source.video = sosg_video_init(filename);
            sosg_video_get_resolution(data->source.video, data->texres);
            break;
        case SOSG_PREDICT:
            data->source.predict = sosg_predict_init(filename);
            sosg_predict_get_resolution(data->source.predict, data->texres);
            break;
    }
    
    if (load_shaders(data)) {
        cleanup(data);
        return 1;
    }
    
    while (handle_events(data) != -1) {
        update_media(data);
        update_display(data);
        timer_update(data);
        data->rotation[0] += data->drotation[0];
        data->rotation[1] += data->drotation[1];
    }
    
    cleanup(data);
	return 0;
}
