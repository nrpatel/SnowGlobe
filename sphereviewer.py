#!/usr/bin/env python
import sys
import pygame
from pygame.locals import *

if len(sys.argv) > 1:
    filename = sys.argv[1]

    pygame.init()
    image = pygame.image.load(filename)
    size = image.get_size()
    display = pygame.display.set_mode(size, pygame.FULLSCREEN)
    display.blit(image, (0, 0))
    pygame.display.flip()
    going = True
    while going:
        events = pygame.event.get()
        for e in events:
            if e.type == QUIT or (e.type == KEYDOWN and e.key == K_ESCAPE):
                going = False
    
else:
    print "Usage: python sphereviewer.py image"
