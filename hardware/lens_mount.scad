opteka_r1 = 24.75/2;
opteka_r2 = 41.35/2;

module rounded(size, r) {
    union() {
        translate([r, 0, 0]) cube([size[0]-2*r, size[1], size[2]]);
        translate([0, r, 0]) cube([size[0], size[1]-2*r, size[2]]);
        translate([r, r, 0]) cylinder(h=size[2], r=r);
        translate([size[0]-r, r, 0]) cylinder(h=size[2], r=r);
        translate([r, size[1]-r, 0]) cylinder(h=size[2], r=r);
        translate([size[0]-r, size[1]-r, 0]) cylinder(h=size[2], r=r);
    }
}

module opteka() {
    union() {
        cylinder(r=23.06/2, h=7);
        translate([0, 0, 3]) cylinder(r=opteka_r1, h=2.5);
        cylinder(r=18.66/2, h=10);
        translate([0, 0, 10]) cylinder(r1=25/2, r2=opteka_r2, h=3);
        translate([0, 0, 13]) cylinder(r=opteka_r2, h=8.42);
    }
}

margin = 0.1;
angle = -12.15;
wall = 5;
wall2 = 2;
image_h = 10;

showwx_offset = 14;
showwx_w = 61.25;
showwx_wb = 54;
showwx_wb2 = 34;
showwx_l1 = 68;
showwx_l2 = 85;

globe_r1 = 98/2;
globe_r2 = 105/2;
globe_h = 15;
globe_bolt = 3.8/2;

module lens_mount() difference() {
    translate([0, 0, -2]) union() {
        translate([0, 0, opteka_r2]) rotate([angle, 0, 0]) difference() {
            union() {
                difference() {
                    translate([-globe_r2-4*wall, -12, -opteka_r2-wall]) cube([2*(globe_r2+4*wall), 2*wall, 4*wall]);
                    translate([-globe_r2-2*wall, -12, -opteka_r2+wall]) rotate([-90, 0, 0]) cylinder(r=globe_bolt, h=20);
                    translate([globe_r2+2*wall, -12, -opteka_r2+wall]) rotate([-90, 0, 0]) cylinder(r=globe_bolt, h=20);
                }
                for(t = [-45, -135]) {
                    rotate([0, t, 0]) translate([0, -12, -2*wall]) difference() {
                        cube([globe_r2+4*wall, 2*wall, 4*wall]);
                        translate([globe_r2+2*wall, 0, 2*wall]) rotate([-90, 0, 0]) cylinder(r=globe_bolt, h=globe_h);
                    }
                }
                translate([-opteka_r1-wall, -12, -opteka_r2-wall]) union() {
                    cube([2*(opteka_r1+wall), 12, (opteka_r2+wall+opteka_r1)]);
                    rounded([2*(opteka_r1+wall), 2*(opteka_r1+wall), 17], 10);
                }
            }
            minkowski() {
                rotate([90, 0, 0]) opteka();
                translate([-margin, -margin, -margin]) cube([margin*2, margin*2, 50+margin]);
            }
        }
        
        
        translate([-showwx_offset, 0, 0]) difference() {
            union() {
                translate([-showwx_wb/2, 0, 0]) rounded([showwx_wb, showwx_l1, opteka_r2-image_h], 10);
                translate([-showwx_wb2/2, 0, 0]) rounded([showwx_wb2, showwx_l2, opteka_r2-image_h], 10);
                for(y = [30, 60]) {
                    union() {
                        translate([-showwx_w/2-wall2, y, 0]) cube([wall2, 10, 20]);
                        translate([showwx_w/2, y, 0]) cube([wall2, 10, 20]);
                        translate([-showwx_w/2, y, 0]) cube([showwx_w, 10, opteka_r2-image_h]);
                    }
                }
            }
            translate([-showwx_wb2/2+wall, wall, 0]) rounded([showwx_wb2-2*wall, showwx_l2-2*wall, 50], 10);
        }
    }
    
    translate([-200, -200, -100]) cube([400, 400, 100]);
}

module globe_mount() {
    difference() {
        union() {
            cylinder(r1=globe_r2+wall, r2=globe_r1+wall, h=globe_h);
            translate([-globe_r2-4*wall, -opteka_r2-wall, 0]) difference() {
                rounded([2*(globe_r2+4*wall), 4*wall, wall],2);
                translate([2*wall, 2*wall, 0]) cylinder(r=globe_bolt, h=globe_h);
                translate([2*globe_r2+6*wall, 2*wall, 0]) cylinder(r=globe_bolt, h=globe_h);
            }
            for(t = [45, 135]) {
                rotate([0, 0, t]) translate([0, -2*wall, 0]) difference() {
                    rounded([globe_r2+4*wall, 4*wall, wall], 2);
                    translate([globe_r2+2*wall, 2*wall, 0]) cylinder(r=globe_bolt, h=globe_h);
                }
            }
            for(i = [0:11]) {
                rotate([0, 0, i*360/12]) translate([-globe_r2-wall, -wall, 0]) cube([20, 2*wall, globe_h/2+wall]);
            }
        }
        cylinder(r1=globe_r2, r2=globe_r1, h=globe_h-wall);
        cylinder(r=globe_r1, h=globe_h+1);
        translate([-wall, -globe_r2-2*wall, 0]) cube([2*wall, 2*(globe_r2+2*wall), globe_h+1]);
        for(i = [0:11]) {
            rotate([0, 0, i*360/12]) translate([-globe_r2-wall, 0, globe_h/2]) rotate([0, 90, 0]) cylinder(r=globe_bolt, h=20);
        }
    }
}

lens_mount();

//globe_mount();
