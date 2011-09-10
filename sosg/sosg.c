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

static SDL_Surface *load_image(const char *filename)
{
    SDL_Surface *surface = IMG_Load(filename);
    SDL_Surface *converted = NULL;
    if (surface) {
        SDL_PixelFormat format = {NULL, 32, 4, 0, 0, 0, 0, 0, 8, 16, 24, 0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000, 0, 255};
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

int main(int argc, char *argv[])
{
    int w = 1440, h = 900;
    SDL_Surface *screen;

    // Slightly different SDL initialization
    if ( SDL_Init(SDL_INIT_VIDEO) != 0 ) {
        printf("Unable to initialize SDL: %s\n", SDL_GetError());
        return 1;
    }

    SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 ); // *new*

    screen = SDL_SetVideoMode( w, h, 32, SDL_OPENGL | SDL_FULLSCREEN ); // *changed*
    if ( !screen ) {
		printf("Unable to set video mode: %s\n", SDL_GetError());
		return 1;
	}
	
    // Set the OpenGL state after creating the context with SDL_SetVideoMode

	glClearColor( 0, 0, 0, 0 );
	
	glEnable( GL_TEXTURE_2D ); // Need this to display a texture

    glViewport( 0, 0, w, h );

    glMatrixMode( GL_PROJECTION );
    glLoadIdentity();

    glOrtho( 0, w, h, 0, -1, 1 );
    
    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity();
    
    // Load the OpenGL texture

    GLuint texture; // Texture object handle
    SDL_Surface *surface; // Gives us the information to make the texture
    
    if ((surface = load_image("2048.jpg"))) {
    
        // Check that the image's width is a power of 2
        if ( (surface->w & (surface->w - 1)) != 0 ) {
            printf("warning: image.bmp's width is not a power of 2\n");
        }
    
        // Also check if the height is a power of 2
        if ( (surface->h & (surface->h - 1)) != 0 ) {
            printf("warning: image.bmp's height is not a power of 2\n");
        }
    
        texture = load_texture(surface);
        SDL_FreeSurface(surface);
    } else {
        printf("SDL could not load image.bmp: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }
    
    // Clear the screen before drawing
	glClear( GL_COLOR_BUFFER_BIT );
    
    // Bind the texture to which subsequent calls refer to
    glBindTexture( GL_TEXTURE_2D, texture );

    glBegin( GL_QUADS );
        // Top-left vertex (corner)
        glTexCoord2i( 0, 0 );
        glVertex3f( 0, 0, 0 );
    
        // Bottom-left vertex (corner)
        glTexCoord2i( 1, 0 );
        glVertex3f( w, 0, 0 );
    
        // Bottom-right vertex (corner)
        glTexCoord2i( 1, 1 );
        glVertex3f( w, h, 0 );
    
        // Top-right vertex (corner)
        glTexCoord2i( 0, 1 );
        glVertex3f( 0, h, 0 );
    glEnd();
	
    SDL_GL_SwapBuffers();
    
    // Wait for 3 seconds to give us a chance to see the image
    SDL_Delay(3000);
    
    // Now we can delete the OpenGL texture and close down SDL
    glDeleteTextures( 1, &texture );
    
    SDL_Quit();
    
	return 0;
}
