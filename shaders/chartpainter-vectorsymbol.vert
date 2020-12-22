#version 450 core
layout (location = 0) in vec2 vertex;
layout (location = 1) in vec4 trans;

uniform mat4 m_p;
uniform float depth;
uniform float windowScale;
uniform vec2 tr;

void main(void) {
  const float a = 1. / windowScale;
  const vec2  pivot = trans.xy;
  const float ca = trans.z;
  const float sa = trans.w;
  const mat2 r = mat2(ca, sa, -sa, ca);
  const vec2 v = a * r * vertex + pivot + tr;
  gl_Position = m_p * vec4(v, depth, 1.);
}

