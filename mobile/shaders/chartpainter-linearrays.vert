#version 320 es
// Original source:
// https://github.com/paulhoux/Cinder-Samples/blob/master/GeometryShader/assets/shaders/lines1.geom
const float MITER_LIMIT = .75;

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
  uint index = uint(gl_VertexID / 2);
  vec4 p0 = m_model * vec4(vertexBufferIn.data[vertexOffset + index], depth, 1.);
  vec4 p1 = m_model * vec4(vertexBufferIn.data[vertexOffset + index + 1U], depth, 1.);
  vec4 p2 = m_model * vec4(vertexBufferIn.data[vertexOffset + index + 2U], depth, 1.);

  // determine the direction of the 2 segments (previous, next)
  vec2 v0 = normalize(p1.xy - p0.xy);
  vec2 v1 = normalize(p2.xy - p1.xy);

  // determine the normal of the 2 segments (previous, next)
  vec2 n0 = vec2(-v0.y, v0.x);
  vec2 n1 = vec2(-v1.y, v1.x);

  if (dot(v0, v1) < - MITER_LIMIT) {

    if (gl_VertexID % 2 == 0) {
      gl_Position = m_p * vec4(p1.xy + HW * (v0 + n0), p1.z, 1.);
    } else {
      gl_Position = m_p * vec4(p1.xy + HW * (v0 - n0), p1.z, 1.);
    }

  } else {
    // determine miter lines by averaging the normals of the 2 segments
    vec2 miter = normalize(n0 + n1);

    // determine the length of the miter from a right angled triangle (miter, n, v)
    float len_m = HW / dot(miter, n0);

    if (gl_VertexID % 2 == 0) {
      gl_Position = m_p * vec4(p1.xy + len_m * miter, p1.z, 1.);
    } else {
      gl_Position = m_p * vec4(p1.xy - len_m * miter, p1.z, 1.);
    }
  }

  if ((gl_VertexID / 2) % 2 == 1) {
    texCoord = length(p2.xy - p1.xy) * windowScale;
  } else {
    texCoord = 0.;
  }
}
