#version 450 core
layout (location = 0) in vec2 vertex;
layout (location = 1) in vec2 texin;

uniform mat4 m_p;
uniform float depth;
uniform float windowScale;
uniform vec2 tr;
uniform vec2 pivot;
uniform vec2 offset;


noperspective out vec2 tex;

void main(void) {
  tex = texin;
  const float a = 1. / windowScale;
  const vec2 v =  a * (vertex + offset) + pivot + tr;
  gl_Position = m_p * vec4(v, depth, 1.);
}

