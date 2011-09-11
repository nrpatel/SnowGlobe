OBJS = sosg_image.o
CC = gcc
CFLAGS = -O3 -Wall `sdl-config --cflags` -DGL_GLEXT_PROTOTYPES
LDFLAGS = -L/usr/X11R6/lib -lGL -lGLU `sdl-config --libs` -lSDL_image

.PHONY: all
all: sosg

sosg: sosg.o $(OBJS)
	$(CC) -o $@ sosg.o $(OBJS) $(CFLAGS) $(LDFLAGS)

.PHONY: clean
clean:
	rm -f sosg_image.o sosg.o sosg
