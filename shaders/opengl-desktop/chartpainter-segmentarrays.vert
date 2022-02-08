#version 450 core
uniform float depth;
uniform uint vertexOffset;
uniform float lineWidth; // the line width in display (mm) units
uniform mat4 m_model;
uniform mat4 m_p;
uniform float windowScale; // transform between display (mm) and chart (m) units

noperspective out float texCoord;
flat out float segmentLength;

layout(std430, binding = 0) buffer VertexBufferIn {
  vec2 data[];
} vertexBufferIn;

void main() {

  const float HW = .5 * lineWidth / windowScale;
  const int si = 2 * (gl_VertexID / 6); // segment index

  const vec4[2] p = {m_model * vec4(vertexBufferIn.data[vertexOffset + si], depth, 1.),
                     m_model * vec4(vertexBufferIn.data[vertexOffset + si + 1], depth, 1.)};

  // direction unit vector
  const vec2 u = normalize(p[1].xy - p[0].xy);
  // ccw normal unit vector
  const vec2 n = vec2(-u.y, u.x);

  int vi = gl_VertexID % 6;
  if (vi > 3) {
    vi = 2 * (vi - 4);
  }

  const float a = vi % 3 == 0 ? HW : - HW;
  const int i = vi > 1 ? 1 : 0;
  gl_Position = m_p * vec4(p[i].xy + a * n, p[0].z, 1.);

  texCoord = i;
  segmentLength = length(p[1].xy - p[0].xy) * windowScale;
}
