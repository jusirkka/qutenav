#version 310 es

uniform vec3 center;
uniform mat4 m_pv;
uniform float angle;
uniform int vertexOffset;

layout(std430, binding = 0) buffer VertexBufferIn {
  vec4 vertices[];
} vertexBufferIn;


void main() {

  vec4 v = vertexBufferIn.vertices[vertexOffset + gl_VertexID / 2];

  if (gl_VertexID % 2 == 0) {

    gl_Position = m_pv * v;

  } else {

    vec3 x = center;
    float a = angle;
    vec3 p = v.xyz;

    vec3 z = normalize(cross(x, p));
    vec3 y = cross(z, x);
    float px = dot(p, x);
    float py = dot(p, y);
    vec3 q =
        (cos(a) * px - sin(a) * py) * x +
        (sin(a) * px + cos(a) * py) * y;

    gl_Position = m_pv * vec4(q, 1.);

  }
}
