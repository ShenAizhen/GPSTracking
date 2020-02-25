#ifdef GL_ES
out vec4 OUT_COLOR_VAR;
#else
#define OUT_COLOR_VAR fragColor
out vec4 fragColor;
#endif

uniform vec4 baseColor;
uniform vec4 lineColor;

flat in int mask;
in vec3 coords;

void main(void){
    OUT_COLOR_VAR =  vec4(baseColor);
}
