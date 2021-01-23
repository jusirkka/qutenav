#version 310 es

precision highp float;

layout (binding = 0) uniform sampler2D atlas;
uniform vec4 base_color;

in vec2 tex;
layout (location = 0) out vec4 color;

void main() {
  float dist = texture(atlas, tex).r;
  float width = fwidth(dist);
  float alpha = smoothstep(.7 - width, .7 + width, dist);
  color = vec4(base_color.rgb, alpha);
}
