#version 450 core

layout (lines) in;
layout (triangle_strip, max_vertices = 4) out;
// layout (line_strip, max_vertices = 2) out;

uniform vec3 center;
uniform mat4 m_pv;
uniform float angle;

void main() {

  const vec3 x = center;
  const float a = angle;

  for (int i = 0; i < 2; i++) {
    const vec3 p = gl_in[i].gl_Position.xyz;
    gl_Position = m_pv * vec4(p, 1.);
    EmitVertex();
    const vec3 z = normalize(cross(x, p));
    const vec3 y = cross(z, x);
    const float px = dot(p, x);
    const float py = dot(p, y);
    const vec3 q =
      (cos(a) * px - sin(a) * py) * x +
      (sin(a) * px + cos(a) * py) * y;
    gl_Position = m_pv * vec4(q, 1.);
    EmitVertex();
  }
  EndPrimitive();
}
