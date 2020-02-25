layout(location = 0) in vec3 verts;
layout(location = 1) in vec3 normals;
layout(location = 2) in mat4 modelMatrix;
uniform mat4 model;
uniform mat4 projection;
uniform mat4 view;
out vec3 vPosition;

void main() {
   vec4 v4 = vec4(verts, 1.0);
   vPosition = vec3(model * v4);
   gl_Position = projection * view * model * v4;
}

