#ifdef GL_ES
in vec4 position;
//in vec3 normals;
uniform highp mat4 model;
uniform highp mat4 view;
uniform highp mat4 projection;
#else
layout(location = 0) in vec4 position;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
#endif

flat out int mask;
out vec3 coords;
void main(void){
    mat4 MV = view * model;
    vec3 eyeSpacePosition = vec3(MV * vec4(position.xyz, 1.0));
    mask = int(position.w);
    coords = position.xyz;
    gl_Position = projection * vec4(eyeSpacePosition, 1.0);
}
