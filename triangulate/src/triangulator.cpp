#include "triangulator.h"
#include "earcut.hpp"

namespace mapbox {
namespace util {

template <>
struct nth<0, glm::vec2> {
  inline static auto get(const glm::vec2 &t) {
    return t.x;
  }
};

template <>
struct nth<1, glm::vec2> {
  inline static auto get(const glm::vec2 &t) {
    return t.y;
  }
};

} // namespace util
} // namespace mapbox


Triangulator::Triangulator(const GL::VertexVector &vertices,
                           const GL::IndexVector &indices)
  : m_vertices(vertices)
  , m_indices(indices)
{
  m_polygon.offset = 0;
  m_polygon.count = 0;
}


void Triangulator::addPolygon(uint offset, size_t count) {
  m_polygon.offset = offset;
  m_polygon.count = count;
}

void Triangulator::addHole(uint offset, size_t count) {
  Extent e;
  e.offset = offset;
  e.count = count;
  m_holes.append(e);
}


GL::IndexVector Triangulator::triangulate() {
  if (m_indices.isEmpty()) {
    return triangulateArrays();
  }

  QVector<QVector<glm::vec2>> polygon;
  GL::IndexVector indexCover;

  const glm::vec2* vs = reinterpret_cast<const glm::vec2*>(m_vertices.constData());

  QVector<glm::vec2> poly;

  for (uint i = 0; i < m_polygon.count; i++) {
    const int index = m_indices[m_polygon.offset + i];
    poly.push_back(vs[index]);
    indexCover << index;
  }
  polygon.push_back(poly);

  for (const Extent& e: m_holes) {
    QVector<glm::vec2> hole;
    for (uint i = 0; i < e.count; i++) {
      const int index = m_indices[e.offset + i];
      hole.push_back(vs[index]);
      indexCover << index;
    }
    polygon.push_back(hole);
  }

  // Three subsequent indices form a triangle. Output triangles are clockwise.
  auto earcuts = mapbox::earcut<GLuint>(polygon);

  // add triangle indices in ccw order
  GL::IndexVector indices;
  for (uint i = 0; i < earcuts.size() / 3; i++)  {
    const GLuint i0 = indexCover[earcuts[3 * i]];
    const GLuint i1 = indexCover[earcuts[3 * i + 1]];
    const GLuint i2 = indexCover[earcuts[3 * i + 2]];
    indices << i0 << i2 << i1;
  }

  return indices;
}

GL::IndexVector Triangulator::triangulateArrays() {
  QVector<QVector<glm::vec2>> polygon;

  const glm::vec2* vs = reinterpret_cast<const glm::vec2*>(m_vertices.constData());

  QVector<glm::vec2> poly;

  for (uint i = 0; i < m_polygon.count; i++) {
    poly.push_back(vs[i]);
  }
  polygon.push_back(poly);

  for (const Extent& e: m_holes) {
    QVector<glm::vec2> hole;
    for (uint i = 0; i < e.count; i++) {
      hole.push_back(vs[i]);
    }
    polygon.push_back(hole);
  }

  // Three subsequent indices form a triangle. Output triangles are clockwise.
  auto earcuts = mapbox::earcut<GLfloat>(polygon);

  GL::IndexVector indices(earcuts.begin(), earcuts.end());
  return indices;
}
