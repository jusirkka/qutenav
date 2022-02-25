#version 450 core

layout (binding = 0) uniform sampler2D atlas;
uniform vec4 base_color;

in vec2 tex;
layout (location = 0) out vec4 color;

const float width = .19;
const float surf = .75;

void main() {
  const float dist = texture(atlas, tex).r;
  const float alpha = smoothstep(surf - width, surf + width, dist);
  color = vec4(base_color.rgb, base_color.a * alpha);
}
