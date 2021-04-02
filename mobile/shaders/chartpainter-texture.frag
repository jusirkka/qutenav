#version 320 es

precision highp float;

layout (binding = 0) uniform sampler2D atlas;

in vec2 tex;
layout (location = 0) out vec4 color;

void main() {
  color = texture(atlas, tex);
}
