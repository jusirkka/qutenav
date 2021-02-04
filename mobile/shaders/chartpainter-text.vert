#version 310 es
layout (location = 0) in vec2 vertex;
layout (location = 1) in vec2 texin;

uniform mat4 m_p;
uniform int w_atlas;
uniform int h_atlas;
uniform float depth;
uniform float windowScale;
uniform float textScale;
uniform mat4 m_model;
uniform vec2 pivot;
uniform vec2 offset;


out vec2 tex;

void main() {
  tex = vec2(texin.x / float(w_atlas), texin.y / float(h_atlas));
  float a = 1. / windowScale;
  vec2 v = a * (textScale * vertex + offset) + pivot;
  gl_Position = m_p * m_model * vec4(v, depth, 1.);
}

