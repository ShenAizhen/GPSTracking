layout(location = 0) in vec3 position;
layout(location = 1) in vec2 tex;
smooth out vec2 texCoords;

void main() {
   texCoords = tex;
   gl_Position = vec4(position, 1.0);
}
