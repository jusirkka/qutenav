#version 320 es


uniform highp vec4 base_color;

in highp float diffuse;

out highp vec4 color;

void main() {
  color = diffuse * base_color;
}
