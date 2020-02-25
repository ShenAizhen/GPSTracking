#ifdef GL_ES
out highp vec4 OUT_COLOR_VAR;
#else
#define OUT_COLOR_VAR fragColor
out vec4 fragColor;
#endif

uniform highp vec4 baseColor;
uniform highp vec4 lineColor;

void main(void){
    OUT_COLOR_VAR =  vec4(0.5607, 0.70588, 0.48627, 0.0);
}
