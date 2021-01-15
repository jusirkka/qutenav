#version 450 core
layout (location = 0) in vec2 vertex;

uniform float depth;
uniform mat4 m_model;

void main(void) {
  gl_Position = m_model * vec4(vertex, depth, 1.);
}
