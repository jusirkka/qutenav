#version 450 core


uniform samplerCube globe;

in float diffuse;
in vec3 texCoord;

out vec4 color;

void main() {
  color = diffuse * texture(globe, texCoord);
}
