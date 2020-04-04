#version 320 es

const float TESS_MAX = 10.;
const float TESS_MIN = .1;
const float D_MIN = 0.0001567855942887398;
const float D_MAX = 0.001567855942887398;
const float RADS_PER_DEG = 0.01745329251994329577;


uniform vec3 eye;

in vec2 coord[];

layout (vertices = 1) out;

patch out vec2 corner[4];


float tess_clamp(in float d) {
  float t = (d - D_MIN) / (D_MAX - D_MIN);
  if (t < 0.) return TESS_MIN;
  if (t > 1.) return TESS_MAX;
  return (TESS_MAX - TESS_MIN) * t + TESS_MIN;
}

float detail(const in vec3 n1, const in vec3 n2) {
  return atan(length(cross(n1, n2)), dot(n1, n2)) / length(0.5 * (n1 + n2) - eye);
}

vec3 normal(const in vec2 p) {
  float lng = p[0] * RADS_PER_DEG;
  float lat = p[1] * RADS_PER_DEG;
  return vec3(cos(lng) * cos(lat),
              sin(lng) * cos(lat),
                         sin(lat));

}

void main() {
  float t1 = tess_clamp(detail(normal(coord[0]), normal(coord[1])));
  float t2 = tess_clamp(detail(normal(coord[1]), normal(coord[2])));
  gl_TessLevelOuter[1] = max(t1, t2);
  gl_TessLevelOuter[0] = 4.;

  corner[0] = coord[0];
  corner[1] = coord[1];
  corner[2] = coord[2];
  corner[3] = coord[3];
}
