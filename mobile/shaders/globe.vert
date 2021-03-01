#version 310 es

layout (location = 0) in vec3 vertex;

uniform mat4 m_pv;
uniform vec3 lp;

out float diffuse;
out vec3 texCoord;

void main() {
  diffuse = max(0., dot(lp, vertex));
  gl_Position = m_pv * vec4(vertex, 1.);
  texCoord = vec3(vertex.x, vertex.z, -vertex.y);
}
