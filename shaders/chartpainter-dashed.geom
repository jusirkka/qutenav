#version 450
// Original source:
// https://github.com/paulhoux/Cinder-Samples/blob/master/GeometryShader/assets/shaders/lines1.geom


const float MITER_LIMIT = .75;

uniform float lineWidth; // the thickness of the line in pixels
uniform mat4 m_p;
uniform float windowScale;

noperspective out float texCoord;

layout(lines_adjacency) in;
layout(triangle_strip, max_vertices = 7) out;

void main(void) {

  const float thickness = lineWidth / windowScale;

  // get the four vertices passed to the shader:
  const vec2 p0 = gl_in[0].gl_Position.xy; // start of previous segment
  const vec2 p1 = gl_in[1].gl_Position.xy; // end of previous segment, start of current segment
  const vec2 p2 = gl_in[2].gl_Position.xy; // end of current segment, start of next segment
  const vec2 p3 = gl_in[3].gl_Position.xy; // end of next segment

  // determine the direction of each of the 3 segments (previous, current, next)
  vec2 v0 = normalize(p1 - p0);
  vec2 v1 = normalize(p2 - p1);
  vec2 v2 = normalize(p3 - p2);

  // determine the normal of each of the 3 segments (previous, current, next)
  vec2 n0 = vec2(-v0.y, v0.x);
  vec2 n1 = vec2(-v1.y, v1.x);
  vec2 n2 = vec2(-v2.y, v2.x);

  // determine miter lines by averaging the normals of the 2 segments
  vec2 miter_a = normalize(n0 + n1);	// miter at start of current segment
  vec2 miter_b = normalize(n1 + n2);	// miter at end of current segment

  // determine the length of the miter from a right angled triangle (miter, n, v)
  float len_a = thickness / dot(miter_a, n1);
  float len_b = thickness / dot(miter_b, n1);

  texCoord = 0.;

  // prevent excessively long miters at sharp corners
  if (dot(v0, v1) < - MITER_LIMIT) {
    miter_a = n1;
    len_a = thickness;

    // close the gap
    if(dot(v0, n1) > 0) {
      gl_Position = m_p * vec4(p1 + thickness * n0, gl_in[1].gl_Position.z, 1.);
      EmitVertex();

      gl_Position = m_p * vec4(p1 + thickness * n1, gl_in[1].gl_Position.z, 1.);
      EmitVertex();

      gl_Position = m_p * vec4(p1, gl_in[1].gl_Position.z, 1.);
      EmitVertex();

      EndPrimitive();
    } else {
      gl_Position = m_p * vec4(p1 - thickness * n1, gl_in[1].gl_Position.z, 1.);
      EmitVertex();

      gl_Position = m_p * vec4(p1 - thickness * n0, gl_in[1].gl_Position.z, 1.);
      EmitVertex();

      gl_Position = m_p * vec4(p1, gl_in[1].gl_Position.z, 1.);
      EmitVertex();

      EndPrimitive();
    }
  }

  if (dot(v1, v2) < -MITER_LIMIT ) {
    miter_b = n1;
    len_b = thickness;
  }

  // generate the triangle strip
  gl_Position = m_p * vec4(p1 + len_a * miter_a, gl_in[1].gl_Position.z, 1.);
  EmitVertex();

  gl_Position = m_p * vec4(p1 - len_a * miter_a, gl_in[1].gl_Position.z, 1.);
  EmitVertex();

  texCoord = windowScale * length(p2 - p1);

  gl_Position = m_p * vec4(p2 + len_b * miter_b, gl_in[2].gl_Position.z, 1.);
  EmitVertex();

  gl_Position = m_p * vec4(p2 - len_b * miter_b, gl_in[2].gl_Position.z, 1.);
  EmitVertex();

  EndPrimitive();
}
