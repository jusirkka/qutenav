#version 450 core
layout (location = 0) in vec2 vertex;

uniform mat4 m_p;
uniform float depth;
uniform vec2 tr;

void main(void) {
  gl_Position = m_p * vec4(vertex + tr, depth, 1.);
}
