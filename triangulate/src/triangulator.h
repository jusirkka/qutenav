#pragma once

#include "types.h"
#include "tessellator.h"

class Triangulator {
public:
  Triangulator(const GL::VertexVector& vertices,
               const GL::IndexVector& indices = GL::IndexVector());

  void addPolygon(uint offset, size_t count);
  void addHole(uint offset, size_t count);
  GL::IndexVector triangulate();

  ~Triangulator();

private:

  Tessellator* m_tess;

};
