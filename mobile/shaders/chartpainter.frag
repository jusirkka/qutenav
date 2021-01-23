#version 310 es

precision mediump float;

uniform vec4 base_color;
out vec4 color;

void main() {
  color = base_color;
}
