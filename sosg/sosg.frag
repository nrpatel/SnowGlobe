uniform sampler2D tex;
uniform float radius;
uniform float height;
uniform float ratio;
uniform vec2 rotation;
uniform vec2 center;
uniform vec2 texres;

#define SIN_PI_4 0.7071067811865475
#define PI2 6.283185307179586
#define PI 3.141592653589793
#define PI_2 1.5707963267948966

void main(void)
{
    vec4 color = vec4(0.0);
    vec2 offset = (gl_TexCoord[0].st - center)*vec2(ratio, 1.0);
    float d = length(offset);
    if (d > radius) {
        gl_FragColor = color;
    } else {
        // Map equirectangular to the Snow Globe fisheye
        float h = d*SIN_PI_4/radius;
        float theta = asin(height*h)+asin(h);
        float phi = atan(offset[0],offset[1]);
        // TODO: rotation in both theta and phi
        vec2 fisheye = vec2((rotation[0]-phi)/PI2, theta/PI_2);
        
        // A really naive filter to reduce sparkling
        color += texture2D(tex, fisheye + vec2(-texres[0], 0.0));
        color += texture2D(tex, fisheye + vec2(texres[0], 0.0));
        color += texture2D(tex, fisheye);
        color += texture2D(tex, fisheye + vec2(0.0, texres[1]));
        color += texture2D(tex, fisheye + vec2(0.0, -texres[1]));
        color /= 5.0;
	    gl_FragColor = color;
	}
}
