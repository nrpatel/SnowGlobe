uniform sampler2D tex;
uniform float globe_radius;
uniform vec2 globe_center;
uniform float globe_offset;

#define SIN_PI_4 0.7071067811865475
#define PI2 6.283185307179586
#define PI 3.141592653589793
#define PI_2 1.5707963267948966

void main(void)
{
    vec2 offset = gl_TexCoord[0].st-vec2(0.5, 0.5);
    float d = length(offset);
    if (d > 0.5) {
        gl_FragColor = vec4(0.0, 0.0, 0.0, 0.0);
    } else {
        float h = d*SIN_PI_4/0.5;
        float theta = asin(h/0.5)+asin(h);
        float phi = atan(offset[0],offset[1]);
        vec2 fisheye = vec2((PI-phi)/PI2, theta/PI_2);
	    gl_FragColor = texture2D(tex, fisheye);
	}
}
