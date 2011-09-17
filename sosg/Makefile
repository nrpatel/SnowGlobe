OBJS = sosg_image.o sosg_video.o sosg_predict.o
CC = gcc
CFLAGS = -O3 -Wall `sdl-config --cflags` -I/usr/local/include/SDL -DGL_GLEXT_PROTOTYPES
LDFLAGS = -lGL -lGLU `sdl-config --libs` -lSDL_image -lSDL_net -lSDL_gfx -l SDL_ttf -lvlc

.PHONY: all
all: sosg

sosg: sosg.o $(OBJS)
	$(CC) -o $@ sosg.o $(OBJS) $(CFLAGS) $(LDFLAGS)

.PHONY: clean
clean:
	rm -f $(OBJS) sosg.o sosg
