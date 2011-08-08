""" Snow Globe: Calibration Script for a Cheap DIY Spherical Projection Setup

Copyright (c) 2011, Nirav Patel <http://eclecti.cc>

Permission to use, copy, modify, and/or distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

This script allows you to calibrate a Snow Globe.  You use the arrow keys to
line up the longitude lines correctly to arrive at the x and y position,
plus and minus keys to adjust the radius size until it fits the full visible
area of the sphere, and 9 and 0 to adjust the lens offset until the latitudes
look properly aligned.  Press s to save the calibration data to a file.

usage:
python calibration.py calibration.txt
"""

#!/usr/bin/env python
import sys
import math
import pickle
import pygame
from pygame.locals import *

class SnowGlobeCalibration(object):
    def __init__(self, fullscreen, filename):
        self.filename = filename
        self.size = (848, 480)
        self.clock = pygame.time.Clock()
        if fullscreen:
            self.display = pygame.display.set_mode(self.size, pygame.FULLSCREEN)
        else:
            self.display = pygame.display.set_mode(self.size, 0)
        self.display.fill((0,0,0))
        self.r = 378
        self.offset = 370.0
        self.center = (431,210)
        self.lastrect = None
        
        if self.filename != None:
            self.open_calibration()
        self.sphere = pygame.surface.Surface((self.r*2,self.r*2))
        self.generate_sphere()
        
        self.dx = 0
        self.dy = 0
        self.dr = 0
        self.do = 0
        self.going = True
        
# An abandoned path by which I went pixel by pixel, determined the correct
# theta and phi angles mapping to the hemisphere, and decided the color based
# on the angle.  Significantly too slow to run real time in python
#    def coord_color(self, theta, phi):
#        r = 255
#        g = 255
#        b = 255
#        if theta < math.pi/4:
#            r = 0
#        if phi > math.pi/2 or (phi > -math.pi/2 and phi < 0):
#            g = 0
#        return (r, g, b)
#        
#    def cartesian_to_polar(self, x, y, d):
#        theta = math.acos(d/self.r)
#        phi = math.atan2(x-self.r,y-self.r)
#        return theta, phi
#        
#    def generate_sphere(self):
#        self.sphere = pygame.surface.Surface((self.r*2, self.r*2))
#        pixels = pygame.surfarray.pixels3d(self.sphere)
#        for x, col in enumerate(pixels):
#            for y, color in enumerate(col):
#                d = math.sqrt((x-self.r)**2+(y-self.r)**2)
#                if d < self.r:
#                    theta, phi = self.cartesian_to_polar(x, y, d)
#                    pixels[x][y] = self.coord_color(theta, phi)
        
    ''' A whole lot of voodoo involving the law of sines to map a sphere
        to a hemisphere and then the hemisphere to a plane. '''
    def radius_for_theta(self, theta):
        side_a = math.sin(theta)*self.r
        side_c = math.sin(math.pi/2.0-theta)*self.r
        # mapping the sphere angle to the hemisphere angle
        angle_b = math.atan2(side_a, side_c + self.offset)
        angle_b = min(math.pi/2,max(0.0,angle_b))
        # map the hemisphere angle to the radius of the projected image in pixels
        # assumes the lens follows the r=2*f*sin(theta/2) mapping, but who knows
        rad = (float(self.r)/math.sin(math.pi/4))*math.sin(angle_b/2.0)
        return int(rad)
        
    def generate_sphere(self):
        self.sphere = pygame.surface.Surface((self.r*2, self.r*2))
        # latitude colored regions
        pygame.draw.circle(self.sphere, (255,0,64), (self.r, self.r), self.radius_for_theta(math.pi))
        pygame.draw.circle(self.sphere, (192,0,128), (self.r, self.r), self.radius_for_theta(math.pi*3.0/4.0))
        pygame.draw.circle(self.sphere, (128,0,192), (self.r, self.r), self.radius_for_theta(math.pi/2.0))
        pygame.draw.circle(self.sphere, (64,0,255), (self.r, self.r), self.radius_for_theta(math.pi/4.0))
        # longitude grid lines
        pygame.draw.line(self.sphere, (128,255,128), (self.r, self.r), (0, 0), 5)
        pygame.draw.line(self.sphere, (128,255,128), (self.r, self.r), (self.r, 0), 5)
        pygame.draw.line(self.sphere, (128,255,128), (self.r, self.r), (self.r*2, 0), 5)
        pygame.draw.line(self.sphere, (128,255,128), (self.r, self.r), (self.r*2, self.r), 5)
        pygame.draw.line(self.sphere, (128,255,128), (self.r, self.r), (self.r*2, self.r*2), 5)
        pygame.draw.line(self.sphere, (128,255,128), (self.r, self.r), (self.r, self.r*2), 5)
        pygame.draw.line(self.sphere, (128,255,128), (self.r, self.r), (0, self.r*2), 5)
        pygame.draw.line(self.sphere, (128,255,128), (self.r, self.r), (0, self.r), 5)
        
    def composite(self):
        self.display.fill((0,0,0),self.lastrect)
        rect = self.display.blit(self.sphere, (self.center[0]-self.r, self.center[1]-self.r))
        pygame.display.update([rect,self.lastrect])
        self.lastrect = rect
        
    def move(self):
        self.center = (self.center[0] + self.dx, self.center[1] + self.dy)
        if self.dr != 0 or self.do != 0:
            self.r += self.dr
            if self.offset < self.r or self.do == -1:
                self.offset += self.do
            elif self.offset > self.r:
                self.offset = self.r
            self.generate_sphere()

    def open_calibration(self):
        try:
            f = open(self.filename, 'r')
        except IOError:
            print "Failed to open %s." % self.filename
        else:
            properties = pickle.load(f)
            try:
                self.r = properties["radius"]
                self.offset = properties["offset"]
                self.center = properties["center"]
            except KeyError:
                print "%s missing properties." % self.filename
            f.close()
            
    def save_calibration(self):
        with open(self.filename, 'w') as f:
            properties = dict(radius=self.r,offset=self.offset,center=self.center)
            pickle.dump(properties,f)
        
    def check_input(self):
        events = pygame.event.get()
        for e in events:
            if e.type == QUIT or (e.type == KEYDOWN and e.key == K_ESCAPE):
                self.going = False
            elif e.type == KEYDOWN:
                if e.key == K_LEFT:
                    self.dx = -1
                elif e.key == K_RIGHT:
                    self.dx = 1
                elif e.key == K_UP:
                    self.dy = -1
                elif e.key == K_DOWN:
                    self.dy = 1
                elif e.key == K_MINUS:
                    self.dr = -1
                elif e.key == K_EQUALS:
                    self.dr = 1
                elif e.key == K_9:
                    self.do = -1
                elif e.key == K_0:
                    self.do = 1
                elif e.key == K_s:
                    if self.filename != None:
                        self.save_calibration()
            elif e.type == KEYUP:
                if (e.key == K_LEFT and self.dx == -1) or (e.key == K_RIGHT and self.dx == 1):
                    self.dx = 0
                elif (e.key == K_UP and self.dy == -1) or (e.key == K_DOWN and self.dy == 1):
                    self.dy = 0
                elif (e.key == K_MINUS and self.dr == -1) or (e.key == K_EQUALS and self.dr == 1):
                    self.dr = 0
                elif (e.key == K_9 and self.do == -1) or (e.key == K_0 and self.do == 1):
                    self.do = 0
        
    def main(self):
        while self.going:
            self.check_input()
            self.move()
            self.composite()
            self.clock.tick(60)
        print (self.r, self.offset, self.center)

if __name__ == '__main__':
    pygame.init()
    filename = None
    if len(sys.argv) > 1:
        filename = sys.argv[1]
    snowglobe = SnowGlobeCalibration(True, filename)
    snowglobe.main()
    
