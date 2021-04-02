#version 320 es

uniform mat4 m_p;
uniform int w_atlas;
uniform int h_atlas;
uniform float depth;
uniform float windowScale;
uniform mat4 m_model;

// pos UL & LR, tex UL & LR, pivot
layout (location = 0) in vec4 v;
layout (location = 1) in vec4 t;
layout (location = 2) in vec2 pivot;

out vec2 tex;

void main() {

  vec2 pos;
  vec2 texin;
  vec2 vs[] = vec2[2](v.xy, v.zw);
  vec2 ts[] = vec2[2](t.xy, t.zw);

  if (gl_VertexID % 3 != 0) {
    pos = vs[gl_VertexID - 1];
    texin = ts[gl_VertexID - 1];
  } else {
    pos.x = vs[gl_VertexID % 2].x;
    pos.y = vs[(gl_VertexID + 1) % 2].y;
    texin.x = ts[gl_VertexID % 2].x;
    texin.y = ts[(gl_VertexID + 1) % 2].y;
  }

  tex = vec2(texin.x / float(w_atlas), texin.y / float(h_atlas));

  vec2 v = pos / windowScale + pivot;
  gl_Position = m_p * m_model * vec4(v, depth, 1.);
}

