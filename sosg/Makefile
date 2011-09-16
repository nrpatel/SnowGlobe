OBJS = sosg_image.o sosg_video.o sosg_predict.o
CC = gcc
CFLAGS = -O3 -Wall `sdl-config --cflags` -DGL_GLEXT_PROTOTYPES
LDFLAGS = -L/usr/X11R6/lib -lGL -lGLU `sdl-config --libs` -lSDL_image -lvlc

.PHONY: all
all: sosg

sosg: sosg.o $(OBJS)
	$(CC) -o $@ sosg.o $(OBJS) $(CFLAGS) $(LDFLAGS)

.PHONY: clean
clean:
	rm -f $(OBJS) sosg.o sosg
