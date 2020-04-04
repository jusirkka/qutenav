#version 320 es

const float RADS_PER_DEG = 0.01745329251994329577;


patch in vec2 corner[3];

layout(triangles, fractional_even_spacing) in;



void main() {
  vec2 p = corner[0] * gl_TessCoord[0] +
           corner[1] * gl_TessCoord[1] +
           corner[2] * gl_TessCoord[2];
  float lng = p[0] * RADS_PER_DEG;
  float lat = p[1] * RADS_PER_DEG;
  vec3 pos = vec3(cos(lng) * cos(lat),
                  sin(lng) * cos(lat),
                             sin(lat));

   gl_Position = vec4(pos, 1.);
}
