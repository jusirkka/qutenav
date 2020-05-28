#version 450 core
layout (location = 0) in vec2 vertex;

uniform mat4 m_pvm;

void main(void) {
  gl_Position = m_pvm * vec4(vertex, 0., 1.);
}
