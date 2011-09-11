CC = gcc
CFLAGS = -O3 -Wall `sdl-config --cflags` -DGL_GLEXT_PROTOTYPES
LDFLAGS = -L/usr/X11R6/lib -lGL -lGLU `sdl-config --libs` -lSDL_image

.PHONY: all
all: sosg

sosg: sosg.o
	$(CC) -o $@ sosg.o $(CFLAGS) $(LDFLAGS)

.PHONY: clean
clean:
	rm -f sosg.o sosg
