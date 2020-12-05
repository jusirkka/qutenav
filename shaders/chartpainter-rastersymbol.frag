#version 450 core

uniform sampler2D atlas;

in vec2 tex;
out vec4 color;

void main() {
  color = texture2D(atlas, tex);
}
