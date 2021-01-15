#pragma once

#include "tessellator.h"
#include <QByteArray>
#include "libtess2/tesselator.h"

struct MemPool {
  MemPool(int r)
    : bytes(r, 0)
    , size(0) {}

  QByteArray bytes;
  int size;
};


class Lib2Tessellator: public Tessellator {
public:
  Lib2Tessellator(const GL::VertexVector& vertices,
                  const GL::IndexVector& indices = GL::IndexVector());

  void addPolygon(uint offset, size_t count);
  void addHole(uint offset, size_t count);
  GL::IndexVector triangulate();

  ~Lib2Tessellator();

private:


  void addContour(uint offset, size_t count, bool reversed);

  const GL::VertexVector m_vertices;
  const GL::IndexVector m_indices;

  TESStesselator* m_tess;
  MemPool m_pool;
  TESSalloc m_alloc;

  GL::IndexVector m_indexCover;
};
