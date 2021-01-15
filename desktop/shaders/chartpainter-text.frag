#version 450 core

layout (binding = 0) uniform sampler2D atlas;
uniform vec4 base_color;

in vec2 tex;
layout (location = 0) out vec4 color;

void main() {
  const float dist = texture2D(atlas, tex).r;
  const float width = fwidth(dist);
  const float alpha = smoothstep(.7 - width, .7 + width, dist);
  color = vec4(base_color.rgb, alpha);
}
