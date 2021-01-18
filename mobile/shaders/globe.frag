#version 310 es

precision mediump float;

uniform vec4 base_color;

in float diffuse;

out vec4 color;

void main() {
  color = diffuse * base_color;
}
