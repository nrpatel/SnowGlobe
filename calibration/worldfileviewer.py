""" Snow Globe: World File Viewer for a Cheap DIY Spherical Projection Setup

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

This script allows you to view equirectangular World Files on a calibrated
Snow Globe.

usage:
python worldfileviewer.py calibration.txt image.jpg
"""

#!/usr/bin/env python
import sys
import math
import pickle
import pygame
from pygame.locals import *

class WorldFileReader(object):
    def __init__(self, image):
        self.image = pygame.image.load(image)
        self.pixels = pygame.surfarray.pixels3d(self.image)
        self.size = self.image.get_size()
        self.rect = self.image.get_rect()
        self.xpixel = 0.0
        self.ypixel = 0.0
        self.xcoord = 0.0
        self.ycoord = 0.0
        self.minx = 9000
        self.maxx = 0

    # Currently unused support for reading in .tfw information for a map   
    def read_tfw(self, tfw):
        try:
            f = open(tfw, 'r')
        except IOError:
            print "Failed to open %s." % self.calibration
        else:
            lines = f.readlines()
            if len(lines) > 5:
                self.xpixel = math.radians(abs(float(lines[0])))
                self.ypixel = math.radians(abs(float(lines[3])))
                self.xcoord = math.radians(abs(float(lines[4])))
                self.ycoord = math.radians(abs(float(lines[5])))
                
            f.close()
        
    def color_for_coordinate(self, theta, phi):
        point = self.polar_to_cartesian(theta, phi)
        return self.pixels[point[0]][point[1]]
        
    def color_for_pixel(self, x, y):
        return self.pixels[point[0]][point[1]]
        
    def polar_to_cartesian(self, theta, phi):
        x = ((2.0*math.pi-(phi+math.pi))/(2.0*math.pi))*self.size[0]
        y = (theta/(math.pi/2.0))*self.size[1]
        x = max(0,min(self.size[0]-1,int(x)))
        y = max(0,min(self.size[1]-1,int(y)))
        return (x, y)
        
    def color_for_rect(self, rect):
        clipped = rect.clip(self.rect)
        return pygame.transform.average_color(self.image, clipped)
        
    def imagesize(self):
        return self.size


class SphereGenerator(object):
    def __init__(self, reader, r, offset, center):
        self.world = reader
        self.worldsize = self.world.imagesize()
        self.r = r
        self.offset = offset
        self.center = center
        self.sphere = pygame.surface.Surface((self.r*2,self.r*2))
        
    def rect_for_points(self, tl, tr, bl, br):
        x0 = min(tl[0],min(tr[0],min(bl[0],br[0])))
        x1 = max(tl[0],max(tr[0],max(bl[0],br[0])))
        y0 = min(tl[1],min(tr[1],min(bl[1],br[1])))
        y1 = max(tl[1],max(tr[1],max(bl[1],br[1])))
        return pygame.Rect(int(x0), int(y0), int(x1-x0)+1, int(y1-y0)+1)
    
    def surface_to_polar(self, x, y, d):
        h = d*math.sin(math.pi/4.0)/self.r
        theta_h = math.asin(h)
        theta_s = math.asin(self.offset*h/self.r)+theta_h
        phi = math.atan2(x-self.r, y-self.r)
        return theta_s, phi
    
    # pixel in the image corresponding to a point on the display
    def pixel_for_point(self, x, y):
        d = math.sqrt((x-self.r)**2+(y-self.r)**2)
        if d < self.r:
            theta, phi = self.surface_to_polar(x, y, d)
            return self.world.polar_to_cartesian(theta, phi)
        else:
            return None
        
    def color_for_pixel(self, x, y):
        d = math.sqrt((x-self.r)**2+(y-self.r)**2)
        if d > self.r:
            # point is outside the projectable area, return black
            return (0, 0, 0)
        
        tl = self.pixel_for_point(float(x)-0.5,float(y)-0.5)
        tr = self.pixel_for_point(float(x)+0.5,float(y)-0.5)
        bl = self.pixel_for_point(float(x)+0.5,float(y)-0.5)
        br = self.pixel_for_point(float(x)+0.5,float(y)+0.5)

        if tl != None and tr != None and bl != None and br != None:
            # get the average color of the region of the map corresponding to the pixel
            rect = self.rect_for_points(tl, tr, bl, br)
            if rect.w > self.worldsize[0]/2 or rect.h > self.worldsize[1]/2:
                # rect spans the width or height of the image, so just use one pixel
                theta, phi = self.surface_to_polar(x, y, d)
                return self.world.color_for_coordinate(theta, phi)
            color = self.world.color_for_rect(rect)
            return (color[0], color[1], color[2])
        else:
            # point is on the edge of the projectable area, so just return one pixel
            theta, phi = self.surface_to_polar(x, y, d)
            return self.world.color_for_coordinate(theta, phi)
        
    def generate_sphere(self):
        self.sphere = pygame.surface.Surface((self.r*2, self.r*2))
        pixels = pygame.surfarray.pixels3d(self.sphere)
        for x, col in enumerate(pixels):
            for y, color in enumerate(col):
                pixels[x][y] = self.color_for_pixel(x, y)
        return self.sphere


class WorldFileViewer(object):
    def __init__(self, fullscreen, calibration, image):
        self.world = WorldFileReader(image)
        self.calibration = calibration
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
        self.dphi = 0.0
        
        if self.calibration != None:
            self.open_calibration()
            
        self.generator = SphereGenerator(self.world, self.r, self.offset, self.center)
        self.sphere = self.generator.generate_sphere()
        self.going = True
    
    def open_calibration(self):
        try:
            f = open(self.calibration, 'r')
        except IOError:
            print "Failed to open %s." % self.calibration
        else:
            properties = pickle.load(f)
            try:
                self.r = properties["radius"]
                self.offset = properties["offset"]
                self.center = properties["center"]
            except KeyError:
                print "%s missing properties." % self.filename
            f.close()

    def move(self):
        pass

    def composite(self):
        self.display.fill((0,0,0),self.lastrect)
        rect = self.display.blit(self.sphere, (self.center[0]-self.r, self.center[1]-self.r))
        pygame.display.update([rect,self.lastrect])
        self.lastrect = rect
            
    def save_image(self):
        pygame.image.save(self.display, "sphere.png")
            
    def check_input(self):
        events = pygame.event.get()
        for e in events:
            if e.type == QUIT or (e.type == KEYDOWN and e.key == K_ESCAPE):
                self.going = False
            elif e.type == KEYDOWN:
                if e.key == K_LEFT:
                    self.dphi = -1.0
                elif e.key == K_RIGHT:
                    self.dphi = 1.0
                elif e.key == K_s:
                    self.save_image()
            elif e.type == KEYUP:
                if (e.key == K_LEFT and self.dphi < 0) or (e.key == K_RIGHT and self.dphi > 0):
                    self.dphi = 0.0
            
    def main(self):
        while self.going:
            self.check_input()
            self.move()
            self.composite()
            self.clock.tick(60)
            
if __name__ == '__main__':
    pygame.init()
    if len(sys.argv) > 2:
        globe = WorldFileViewer(True, sys.argv[1], sys.argv[2])
        globe.main()
    else:
        print "usage: python worldfileviewer.py calibration_file image"
