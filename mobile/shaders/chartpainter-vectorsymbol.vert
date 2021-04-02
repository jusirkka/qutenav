#version 320 es
layout (location = 0) in vec2 vertex;
layout (location = 1) in vec4 trans;

uniform mat4 m_p;
uniform float depth;
uniform float windowScale;
uniform mat4 m_model;

void main() {
  float a = 1. / windowScale;
  vec2  pivot = trans.xy;
  float ca = trans.z;
  float sa = trans.w;
  mat2 r = mat2(ca, sa, -sa, ca);
  vec2 v = a * r * vertex + pivot;
  gl_Position = m_p * m_model * vec4(v, depth, 1.);
}

