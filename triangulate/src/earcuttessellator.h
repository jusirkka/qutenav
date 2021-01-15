#pragma once

#include "tessellator.h"

class EarcutTessellator: public Tessellator {
public:
  EarcutTessellator(const GL::VertexVector& vertices,
                    const GL::IndexVector& indices = GL::IndexVector());

  void addPolygon(uint offset, size_t count);
  void addHole(uint offset, size_t count);
  GL::IndexVector triangulate();

private:

  const GL::VertexVector m_vertices;
  const GL::IndexVector m_indices;

  QVector<QVector<glm::vec2>> m_polygon;
  GL::IndexVector m_indexCover;

};
