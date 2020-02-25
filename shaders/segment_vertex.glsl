#ifdef GLS_ES
in vec3 verts;
in vec3 normals;
#else
layout(location=0) in vec3 verts;
layout(location=1) in vec3 normals;
#endif

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;
out vec3 vPosition;
void main() {
    vec4 v4 = vec4(verts, 1.0);
    vPosition = vec3(model * v4);
    gl_Position = projection * view * model * v4;
}
