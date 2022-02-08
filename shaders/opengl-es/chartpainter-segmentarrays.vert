#version 320 es
uniform float depth;
uniform uint vertexOffset;
uniform float lineWidth; // the line width in display (mm) units
uniform mat4 m_model;
uniform mat4 m_p;
uniform float windowScale; // transform between display (mm) and chart (m) units

out float texCoord;

layout(std430, binding = 0) buffer VertexBufferIn {
  vec2 data[];
} vertexBufferIn;

void main() {

  float HW = .5 * lineWidth / windowScale;
  uint si = 2U * uint(gl_VertexID / 6); // segment index

  vec4[2] p;
  p[0] = m_model * vec4(vertexBufferIn.data[vertexOffset + si], depth, 1.);
  p[1] = m_model * vec4(vertexBufferIn.data[vertexOffset + si + 1U], depth, 1.);

  // direction unit vector
  vec2 u = normalize(p[1].xy - p[0].xy);
  // ccw normal unit vector
  vec2 n = vec2(-u.y, u.x);

  int vi = gl_VertexID % 6;
  if (vi > 3) {
    vi = 2 * (vi - 4);
  }

  float a = vi % 3 == 0 ? HW : - HW;
  int i = vi > 1 ? 1 : 0;
  gl_Position = m_p * vec4(p[i].xy + a * n, p[0].z, 1.);

  if (i == 1) {
    // texCoord = length(m_p * (p2 - p1));
    texCoord = length(p[1].xy - p[0].xy) * windowScale;
  } else {
    texCoord = 0.;
  }
}
