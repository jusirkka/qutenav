#version 450 core
layout (location = 0) in vec2 vertex;
layout (location = 1) in vec2 texin;

uniform mat4 m_pv;
uniform float depth;
uniform float windowScale;
uniform vec2 tr;
uniform vec2 pivot;
uniform vec2 pivotShift;


noperspective out vec2 tex;

void main(void) {
  tex = texin;
  const vec2 v = 1. / windowScale * vertex +
      pivot + tr + pivotShift / windowScale;
  gl_Position = m_pv * vec4(v, depth, 1.);
}

