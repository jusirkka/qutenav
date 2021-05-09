#version 320 es

layout (location = 0) in vec4 corners;

uniform mat4 m_pv;
uniform float angle;
uniform int nump; // number of points per side. #vertices = 2 * (4 * nump + 1)

const float D2R = 0.017453292519943295;
const float R2D = 57.29577951308232;

vec3 pos(float lng, float lat) {
  return vec3(cos(lng) * cos(lat), sin(lng) * cos(lat), sin(lat));
}


void main() {


  vec2 sw = D2R * corners.xy;
  vec2 ne = D2R * corners.zw;

  float c_lng = .5 * (sw.x + ne.x);
  float c_lat = .5 * (sw.y + ne.y);


  vec3 p;

  int side = (gl_VertexID / 2 / nump) % 4;
  int index = (gl_VertexID / 2 ) % nump;

  float t = float(index) / float(nump);
  if (side % 2 == 0) {
    if (side == 0) {
      p = pos(sw.x, (1. - t) * ne.y + t * sw.y);
    } else {
      p = pos(ne.x, (1. - t) * sw.y + t * ne.y);
    }
  } else {
    if (side == 1) {
      p = pos((1. - t) * sw.x + t * ne.x, sw.y);
    } else {
      p = pos((1. - t) * ne.x + t * sw.x, ne.y);
    }
  }


  if (gl_VertexID % 2 == 0) {

    gl_Position = m_pv * vec4(p, 1.);

  } else {

    vec3 center = pos(c_lng, c_lat);
    vec3 x;

    if (index == 0) {
      x = center;
    } else {
      if (side % 2 == 0) {
        if (side == 0) {
          x = pos(c_lng, (1. - t) * ne.y + t * sw.y);
        } else {
          x = pos(c_lng, (1. - t) * sw.y + t * ne.y);
        }
      } else {
        if (side == 1) {
          x = pos((1. - t) * sw.x + t * ne.x, c_lat);
        } else {
          x = pos((1. - t) * ne.x + t * sw.x, c_lat);
        }
      }
    }

    float a = angle;

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
