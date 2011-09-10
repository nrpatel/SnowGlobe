CC = gcc
CFLAGS = -O3 -Wall `sdl-config --cflags`
LDFLAGS = -L/usr/X11R6/lib -lGL -lGLU -lglut `sdl-config --libs` -lSDL_image

.PHONY: all
all: sosg

sosg: sosg.o
	$(CC) -o $@ sosg.o $(CFLAGS) $(LDFLAGS)

.PHONY: clean
clean:
	rm -f sosg.o sosg
