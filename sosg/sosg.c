/* Science on a Snow Globe
 * by Nirav Patel <nrp@eclecti.cc> http://eclecti.cc
 *
 * An extremely basic take on supporting Science On a Sphere datasets on
 * Snow Globe, a low cost DIY spherical display.
 * Datasets and SOS information available at http://sos.noaa.gov/
 *
 * Parts are based on/copied from some public domain code from
 * Kyle Foley: http://gpwiki.org/index.php/SDL:Tutorials:Using_SDL_with_OpenGL
 * John Tsiombikas: http://nuclear.mutantstargoat.com/articles/sdr_fract/
 */

#include "SDL.h"
#include "SDL_opengl.h"
#include "SDL_image.h"

#include <stdio.h>
#include <stdlib.h>

#define TICK_INTERVAL 16
#define PI 3.141592653589793
#define ROTATION_INTERVAL PI/(120.0*(1000.0/TICK_INTERVAL))

typedef struct sosg_struct {
    int w;
    int h;
    int texres[2];
    float radius;
    float height;
    float center[2];
    float rotation;
    float drotation;
    Uint32 time;
    SDL_Surface *screen;
    GLuint texture;
    GLuint program;
    GLuint vertex;
    GLuint fragment;
    GLuint lrotation;
} sosg_t;

static SDL_Surface *load_image(const char *filename)
{
    SDL_Surface *surface = IMG_Load(filename);
    SDL_Surface *converted = NULL;
    if (surface) {
        SDL_PixelFormat format = {NULL, 32, 4, 0, 0, 0, 0, 0, 8, 16, 24, 
            0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000, 0, 255};
        converted = SDL_ConvertSurface(surface, &format, SDL_SWSURFACE);
        SDL_FreeSurface(surface);
    }

    return converted;
}

static GLuint load_texture(SDL_Surface *surface)
{
    GLuint texture;
    
    // Have OpenGL generate a texture object handle for us
    glGenTextures(1, &texture);

    // Bind the texture object
    glBindTexture(GL_TEXTURE_2D, texture);
    
    // Set the texture's stretching properties
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    // Edit the texture object's image data using the information SDL_Surface gives us
    glTexImage2D(GL_TEXTURE_2D, 0, 4, surface->w, surface->h, 0, 
                  GL_RGBA, GL_UNSIGNED_BYTE, surface->pixels);
    
    return texture;
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

static int load_shaders(sosg_t *data)
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

static int setup(sosg_t *data)
{
    // Slightly different SDL initialization
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        printf("Unable to initialize SDL: %s\n", SDL_GetError());
        return 1;
    }
    
    data->time = SDL_GetTicks();
    SDL_EnableKeyRepeat(250, TICK_INTERVAL);
    
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    data->screen = SDL_SetVideoMode(data->w, data->h, 32, SDL_OPENGL);//| SDL_FULLSCREEN);
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
    
    return 0;
}

static void timer_update(sosg_t *data)
{
    Uint32 now = SDL_GetTicks();

    if (data->time > now) {
        SDL_Delay(data->time - now);
    }

    while (data->time <= now) {
        data->time += TICK_INTERVAL;
    }
}

static int handle_events(sosg_t *data)
{
    SDL_Event event;
    
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym) {
                    case SDLK_ESCAPE:
                        return -1;
                    case SDLK_LEFT:
                        data->drotation += ROTATION_INTERVAL;
                        break;
                    case SDLK_RIGHT:
                        data->drotation -= ROTATION_INTERVAL;
                        break;
                    case SDLK_p:
                        data->drotation = 0.0;
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

static void update(sosg_t *data)
{
    glUniform1f(data->lrotation, data->rotation);

    // Clear the screen before drawing
	glClear(GL_COLOR_BUFFER_BIT);
    
    // Bind the texture to which subsequent calls refer to
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, data->texture);

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

int main(int argc, char *argv[])
{
    char *filename = "2048.jpg";
    SDL_Surface *surface;
    sosg_t *data = calloc(1, sizeof(sosg_t));
    
    if (!data) {
        printf("Could not allocate data\n");
        return 1;
    }
    
    data->w = 848;
    data->h = 480;
    data->radius = 378.0;
    data->height = 370.0;
    data->center[0] = 431.0;
    data->center[1] = 210.0;
    data->rotation = PI;

    if (setup(data)) {
        return 1;
    }
    
    if ((surface = load_image(filename))) {
        // Check that the image's dimensions are a power of 2
        if ((surface->w & (surface->w - 1)) != 0 ||
            (surface->h & (surface->h - 1)) != 0) {
            printf("warning: %s's dimensions (%d, %d) not a power of 2\n",
                filename, surface->w, surface->h);
        }
    
        data->texture = load_texture(surface);
        data->texres[0] = surface->w;
        data->texres[1] = surface->h;
        SDL_FreeSurface(surface);
    } else {
        printf("SDL could not load %s: %s\n", filename, SDL_GetError());
        SDL_Quit();
        return 1;
    }
    
    if (load_shaders(data)) {
        SDL_Quit();
        return 1;
    }
    
    while (handle_events(data) != -1) {
        update(data);
        timer_update(data);
        data->rotation += data->drotation;
    }
    
    // Now we can delete the OpenGL texture and close down SDL
    glDeleteTextures(1, &data->texture);
    SDL_Quit();
    
	return 0;
}
