#ifdef GL_ES
in vec3 position;
in vec3 normals;
in mat4 modelMatrix;
uniform mat4 view;
uniform mat4 projection;
#else
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normals;
layout(location = 2) in mat4 modelMatrix;
uniform mat4 view;
uniform mat4 projection;
#endif

void main(void){
    mat4 MV = view * modelMatrix;
    vec3 eyeSpacePosition = vec3(MV * vec4(position, 1.0));
    gl_Position = projection * vec4(eyeSpacePosition, 1.0);
}
