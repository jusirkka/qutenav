#version 450 core
// Original source:
// https://github.com/paulhoux/Cinder-Samples/blob/master/GeometryShader/assets/shaders/lines1.geom
const float MITER_LIMIT = .75;

uniform float depth;
uniform uint vertexOffset;
uniform float lineWidth; // the thickness of the line in pixels
uniform mat4 m_model;
uniform mat4 m_p;
uniform float windowScale;

out float texCoord;

layout(packed, binding = 0) buffer VertexBufferIn {
  vec2 data[];
} vertexBufferIn;

void main(void) {

  const float thickness = lineWidth / windowScale;

  const vec4 p0 = m_model * vec4(vertexBufferIn.data[vertexOffset + gl_VertexID / 2], depth, 1.);
  const vec4 p1 = m_model * vec4(vertexBufferIn.data[vertexOffset + gl_VertexID / 2 + 1], depth, 1.);
  const vec4 p2 = m_model * vec4(vertexBufferIn.data[vertexOffset + gl_VertexID / 2 + 2], depth, 1.);

  // determine the direction of the 2 segments (previous, next)
  const vec2 v0 = normalize(p1.xy - p0.xy);
  const vec2 v1 = normalize(p2.xy - p1.xy);

  // determine the normal of the 2 segments (previous, next)
  const vec2 n0 = vec2(-v0.y, v0.x);
  const vec2 n1 = vec2(-v1.y, v1.x);

  if (dot(v0, v1) < - MITER_LIMIT) {

    if (gl_VertexID % 2 == 0) {
      gl_Position = m_p * vec4(p1.xy + thickness * (v0 + n0), p1.z, 1.);
    } else {
      gl_Position = m_p * vec4(p1.xy + thickness * (v0 - n0), p1.z, 1.);
    }

  } else {
    // determine miter lines by averaging the normals of the 2 segments
    vec2 miter = normalize(n0 + n1);

    // determine the length of the miter from a right angled triangle (miter, n, v)
    float len_m = thickness / dot(miter, n0);

    if (gl_VertexID % 2 == 0) {
      gl_Position = m_p * vec4(p1.xy + len_m * miter, p1.z, 1.);
    } else {
      gl_Position = m_p * vec4(p1.xy - len_m * miter, p1.z, 1.);
    }
  }

  texCoord = length(p2.xy - p1.xy);
}
