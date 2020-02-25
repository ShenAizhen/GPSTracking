#ifdef GL_ES
out vec4 OUT_COLOR_VAR;
uniform highp vec4 baseColor;
uniform highp vec4 lineColor;
uniform highp int zoomLevel;
#else
#define OUT_COLOR_VAR fragColor
out vec4 fragColor;

uniform vec4 baseColor;
uniform vec4 lineColor;
uniform int zoomLevel;
#endif
in vec3 vPosition;

float get_grad_scale(){
    if(zoomLevel < 3) return 0.05;
    if(zoomLevel < 5) return 0.07;
    return 0.1;
}

void main(void){
    vec2 stepSize = vec2(8.5);
    vec2 coord = vPosition.xz / stepSize;
    vec2 frac = fract(coord);
    float grad = get_grad_scale();
    vec2 mult = smoothstep(0.0, grad, frac) - smoothstep(1.0-grad, 1.0, frac);
    vec3 col = mix(lineColor.rgb, baseColor.rgb, mult.x * mult.y);
    OUT_COLOR_VAR = vec4(col, 0.0);
}
