#include "triangulator.h"
#include "earcuttessellator.h"

Triangulator::Triangulator(const GL::VertexVector &vertices,
                           const GL::IndexVector &indices)
  : m_tess(new EarcutTessellator(vertices, indices))
{}


void Triangulator::addPolygon(uint offset, size_t count) {
  m_tess->addPolygon(offset, count);
}

void Triangulator::addHole(uint offset, size_t count) {
  m_tess->addHole(offset, count);
}


GL::IndexVector Triangulator::triangulate() {
  return m_tess->triangulate();
}

Triangulator::~Triangulator() {
  delete m_tess;
}
