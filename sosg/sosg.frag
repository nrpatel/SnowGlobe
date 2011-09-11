uniform sampler2D tex;
uniform float radius;
uniform float height;
uniform float ratio;
uniform vec2 center;

#define SIN_PI_4 0.7071067811865475
#define PI2 6.283185307179586
#define PI 3.141592653589793
#define PI_2 1.5707963267948966

void main(void)
{
    vec2 offset = (gl_TexCoord[0].st - center)*vec2(ratio, 1.0);
    float d = length(offset);
    if (d > radius) {
        gl_FragColor = vec4(0.0, 0.0, 0.0, 0.0);
    } else {
        float h = d*SIN_PI_4/radius;
        float theta = asin(height*h/radius)+asin(h);
        float phi = atan(offset[0],offset[1]);
        vec2 fisheye = vec2((PI-phi)/PI2, theta/PI_2);
	    gl_FragColor = texture2D(tex, fisheye);
	}
}
