#version 320 es
layout(triangle_strip, max_vertices = 3) out;
layout(triangles) in;

uniform mat4 m_pv; // projection * view
uniform vec3 lp; // light direction

out float diffuse;

void main() {

  vec3 p1 = vec3(gl_in[0].gl_Position);
  vec3 p2 = vec3(gl_in[1].gl_Position);
  vec3 p3 = vec3(gl_in[2].gl_Position);

  gl_Position = m_pv * gl_in[0].gl_Position;
  diffuse = max(0., dot(lp, p1));
  EmitVertex();
  if (dot(p1, cross(p2 - p1, p3 - p1)) > 0.) {
    // no change
    gl_Position = m_pv * gl_in[1].gl_Position;
    diffuse = max(0., dot(lp, p2));
    EmitVertex();
    gl_Position = m_pv * gl_in[2].gl_Position;
    diffuse = max(0., dot(lp, p3));
    EmitVertex();
  } else {
    // reversed order
    gl_Position = m_pv * gl_in[2].gl_Position;
    diffuse = max(0., dot(lp, p3));
    EmitVertex();
    gl_Position = m_pv * gl_in[1].gl_Position;
    diffuse = max(0., dot(lp, p2));
    EmitVertex();
  }

  EndPrimitive();

}
