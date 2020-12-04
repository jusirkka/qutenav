#version 450 core

uniform sampler2D atlas;
uniform vec4 base_color;

in vec2 tex;
out vec4 color;

void main() {
  const float dist = texture2D(atlas, tex).r;
  const float width = fwidth(dist);
  const float alpha = smoothstep(.7 - width, .7 + width, dist);
  color = vec4(base_color.rgb, alpha);
}
