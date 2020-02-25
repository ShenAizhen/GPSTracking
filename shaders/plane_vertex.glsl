layout(location=0) in vec3 verts;
layout(location=1) in vec3 normals;
layout(location=2) in vec3 nearVerts;
layout(location=3) in vec3 farVerts;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;
out vec3 vPosition;
out vec3 vNear;
out vec3 vFar;

void main() {
   vec4 v4 = vec4(verts, 1.0);
   vPosition = verts;
   vNear = nearVerts;
   vFar = farVerts;
   gl_Position = v4;
}
