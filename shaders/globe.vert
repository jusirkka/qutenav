#version 450 core
layout (location = 0) in vec3 vertex;

uniform mat4 m_pv;
uniform vec3 lp;

out float diffuse;

void main(void) {
  diffuse = max(0., dot(lp, vertex));
  gl_Position = m_pv * vec4(vertex, 1.);
}
