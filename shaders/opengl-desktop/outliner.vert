#version 450 core
layout (location = 0) in vec3 vertex;

uniform mat4 m_pv;

void main() {
  gl_Position = m_pv * vec4(vertex, 1.);
}
