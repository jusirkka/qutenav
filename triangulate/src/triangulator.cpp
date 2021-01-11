#include "triangulator.h"
#include "earcut.hpp"

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

  std::vector<std::vector<Point>> polygon;
  GL::IndexVector indexCover;

  std::vector<Point> poly;
  for (uint i = 0; i < m_polygon.count; i++) {
    const int index = m_indices[m_polygon.offset + i];
    const Point p{m_vertices[2 * index], m_vertices[2 * index + 1]};
    poly.push_back(p);
    indexCover << index;
  }
  polygon.push_back(poly);

  for (const Extent& e: m_holes) {
    std::vector<Point> hole;
    for (uint i = 0; i < e.count; i++) {
      const int index = m_indices[e.offset + i];
      const Point p{m_vertices[2 * index], m_vertices[2 * index + 1]};
      hole.push_back(p);
      indexCover << index;
    }
    polygon.push_back(hole);
  }

  // Three subsequent indices form a triangle. Output triangles are clockwise.
  std::vector<GLuint> earcuts = mapbox::earcut<GLuint>(polygon);

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
  std::vector<std::vector<Point>> polygon;

  std::vector<Point> poly;
  for (uint i = 0; i < m_polygon.count; i++) {
    const Point p{m_vertices[2 * i], m_vertices[2 * i + 1]};
    poly.push_back(p);
  }
  polygon.push_back(poly);

  for (const Extent& e: m_holes) {
    std::vector<Point> hole;
    for (uint i = 0; i < e.count; i++) {
      const Point p{m_vertices[2 * i], m_vertices[2 * i + 1]};
      hole.push_back(p);
    }
    polygon.push_back(hole);
  }

  // Three subsequent indices form a triangle. Output triangles are clockwise.
  std::vector<GLfloat> earcuts = mapbox::earcut<GLfloat>(polygon);

  GL::IndexVector indices(earcuts.begin(), earcuts.end());
  return indices;
}
