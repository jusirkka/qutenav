#version 450 core
layout (location = 0) in vec2 vertex;

uniform mat4 m_p;
uniform float depth;
uniform mat4 m_model;

void main() {
  gl_Position = m_p * m_model * vec4(vertex, depth, 1.);
}
