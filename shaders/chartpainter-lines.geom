#version 450
// Original source:
// https://github.com/paulhoux/Cinder-Samples/blob/master/GeometryShader/assets/shaders/lines2.geom

// This version of the line shader simply cuts off the corners and
// draws the line with no overdraw on neighboring segments at all

uniform float lineWidth; // the thickness of the line in pixels
uniform float screenWidth; // the size of the viewport in pixels
uniform float screenHeight;

layout(lines_adjacency) in;
layout(triangle_strip, max_vertices = 5) out;

noperspective out float texCoord;

// assuming orthoprojection
vec2 toScreenSpace(vec4 v) {
  return vec2(v.x * screenWidth, v.y * screenHeight);
}

void main(void) {
  // get the four vertices passed to the shader:
  vec2 p0 = toScreenSpace(gl_in[0].gl_Position); // start of previous segment
  vec2 p1 = toScreenSpace(gl_in[1].gl_Position); // end of previous segment, start of current segment
  vec2 p2 = toScreenSpace(gl_in[2].gl_Position); // end of current segment, start of next segment
  vec2 p3 = toScreenSpace(gl_in[3].gl_Position); // end of next segment

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
  miter_a *= lineWidth / dot(miter_a, n1);
  miter_b *= lineWidth / dot(miter_b, n1);

  const vec2 scale = vec2(screenWidth, screenHeight);
  texCoord = 0.;
  if (dot(v0, n1) > 0) {
    // start at negative miter
    gl_Position = vec4((p1 - miter_a) / scale, gl_in[1].gl_Position.z, 1.);
    EmitVertex();

   // proceed to positive normal
   gl_Position = vec4((p1 + lineWidth * n1) / scale, gl_in[1].gl_Position.z, 1.);
   EmitVertex();
 } else {
   // start at negative normal
   gl_Position = vec4((p1 - lineWidth * n1) / scale, gl_in[1].gl_Position.z, 1.0);
   EmitVertex();

   // proceed to positive miter
   gl_Position = vec4((p1 + miter_a) / scale, gl_in[1].gl_Position.z, 1.);
   EmitVertex();
 }

  texCoord = length(p2 - p1);
  if (dot(v2, n1) < 0) {
    // proceed to negative miter
    gl_Position = vec4((p2 - miter_b) / scale, gl_in[2].gl_Position.z, 1.);
    EmitVertex();

    // proceed to positive normal
    gl_Position = vec4((p2 + lineWidth * n1) / scale, gl_in[2].gl_Position.z, 1.);
    EmitVertex();

    // end at positive normal
    gl_Position = vec4((p2 + lineWidth * n2) / scale, gl_in[2].gl_Position.z, 1.);
    EmitVertex();
  } else {
    // proceed to negative normal
    gl_Position = vec4((p2 - lineWidth * n1) / scale, gl_in[2].gl_Position.z, 1.);
    EmitVertex();

    // proceed to positive miter
    gl_Position = vec4((p2 + miter_b) / scale, gl_in[2].gl_Position.z, 1.);
    EmitVertex();

    // end at negative normal
    gl_Position = vec4((p2 - lineWidth * n2) / scale, gl_in[2].gl_Position.z, 1.);
    EmitVertex();
  }
  EndPrimitive();
}
