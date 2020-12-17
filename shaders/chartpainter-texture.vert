#version 450 core
layout (location = 0) in vec2 vertex;
layout (location = 1) in vec2 texin;

uniform mat4 m_pv;
uniform vec2 tr_tex;
uniform vec2 tr_vertex;
uniform vec2 scale_vertex;
uniform vec2 scale_tex;


noperspective out vec2 tex;

void main(void) {
  tex = tr_tex + scale_tex * texin;
  const vec2 v =  tr_vertex + scale_vertex * vertex;
  // place it halfway in visible depth range (-1, 0)
  gl_Position = m_pv * vec4(v, -.5, 1.);
}

