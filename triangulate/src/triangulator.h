#pragma once

#include "types.h"

class Triangulator {
public:
  Triangulator(const GL::VertexVector& vertices,
               const GL::IndexVector& indices = GL::IndexVector());
  void addPolygon(uint offset, size_t count);
  void addHole(uint offset, size_t count);
  GL::IndexVector triangulate();

private:

  GL::IndexVector triangulateArrays();

  const GL::VertexVector m_vertices;
  const GL::IndexVector m_indices;

  struct Extent {
    uint offset;
    size_t count;
  };

  using ExtentVector = QVector<Extent>;

  Extent m_polygon;
  ExtentVector m_holes;

  using Point = std::array<GLfloat, 2>;


};
