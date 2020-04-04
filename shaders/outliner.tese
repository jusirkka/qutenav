#version 320 es

const float RADS_PER_DEG = 0.01745329251994329577;

uniform mat4 m_pv; // projection * view

patch in vec2 corner[4];

layout(isolines, fractional_even_spacing) in;



void main() {

  int i1 = int(gl_TessCoord.y * gl_TessLevelOuter[0] + 0.001);
  int i2 = (i1 + 1) % int(gl_TessLevelOuter[0]);
  float t = gl_TessCoord.x;

  vec2 p = corner[i1] * (1. - t) + corner[i2] * t;

  float lng = p[0] * RADS_PER_DEG;
  float lat = p[1] * RADS_PER_DEG;
  vec4 pos = vec4(cos(lng) * cos(lat),
                  sin(lng) * cos(lat),
                             sin(lat),
                  1.);

   gl_Position = m_pv * pos;
}
