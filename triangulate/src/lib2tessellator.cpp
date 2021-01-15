#include "lib2tessellator.h"
#include <QDebug>


void* memAlloc(void* userData, uint s) {

  auto pool = reinterpret_cast<MemPool*>(userData);

  s = (s + 7) & ~0x7;

  if (pool->size + s > static_cast<uint>(pool->bytes.size())) {
    qWarning() << "Out of memory";
    return nullptr;
  }

  char* ptr = pool->bytes.data() + pool->size;
  pool->size += s;

  return ptr;
}

void memFree(void* /*userData*/, void* /*ptr*/) {
  // empty
}


void* stdAlloc(void* /*userData*/, unsigned int size) {
  return malloc(size);
}

void stdFree(void* /*userData*/, void* ptr) {
  free(ptr);
}

static int factor(int n) {
  int lg = 0;
  int f = (n + 9999) / 10000;
  while (f >>= 1) ++lg;
  return 1 << lg;
}

Lib2Tessellator::Lib2Tessellator(const GL::VertexVector &vertices,
                                 const GL::IndexVector &indices)
  : m_vertices(vertices)
  , m_indices(indices)
  , m_pool(qMax(vertices.size() * 256, 1024*1024))
{


  //  memset(&m_alloc, 0, sizeof(m_alloc));
  //  m_alloc.memalloc = stdAlloc;
  //  m_alloc.memfree = stdFree;
  //  m_alloc.extraVertices = 256;

  int multiplier = factor(vertices.size());
  m_alloc.memalloc = memAlloc;
  m_alloc.memrealloc = nullptr;
  m_alloc.memfree = memFree;
  m_alloc.userData = reinterpret_cast<void*>(&m_pool);
  m_alloc.meshEdgeBucketSize = 512 * multiplier;
  m_alloc.meshVertexBucketSize = 512 * multiplier;
  m_alloc.meshFaceBucketSize = 256 * multiplier;
  m_alloc.dictNodeBucketSize = 512 * multiplier;
  m_alloc.regionBucketSize = 256 * multiplier;
  m_alloc.extraVertices = 256;

  m_tess = tessNewTess(&m_alloc);
}

Lib2Tessellator::~Lib2Tessellator() {
  tessDeleteTess(m_tess);
}


static bool windingCW(const glm::vec2* vs, size_t count) {
  double sum = 0.;

  for (uint i = 0; i < count - 1; i++) {
    sum += vs[i].x * vs[i + 1].y - vs[i].y * vs[i + 1].x;
  }

  sum += vs[count - 1].x * vs[0].y - vs[count - 1].y * vs[0].x;

  return sum < 0.;
}

void Lib2Tessellator::addContour(uint offset, size_t count, bool reversed) {
  const glm::vec2* vs = reinterpret_cast<const glm::vec2*>(m_vertices.constData());

  if (m_indices.isEmpty()) {
    bool cw = windingCW(vs + offset, count);
    if (reversed) cw = !cw;

    tessSetOption(m_tess, TESS_REVERSE_CONTOURS, cw);
    tessAddContour(m_tess, 2, vs + offset, sizeof(GLfloat) * 2, count);

    for (uint i = 0; i < count; i++) m_indexCover << offset + i;

  } else {
    QVector<glm::vec2> poly;
    for (uint i = 0; i < count; i++) {
      const int index = m_indices[offset + i];
      poly.push_back(vs[index]);
      m_indexCover << index;
    }

    bool cw = windingCW(poly.constData(), count);
    if (reversed) cw = !cw;

    tessSetOption(m_tess, TESS_REVERSE_CONTOURS, false);
    tessAddContour(m_tess, 2, poly.constData(), sizeof(GLfloat) * 2, count);
  }
}

void Lib2Tessellator::addPolygon(uint offset, size_t count) {
  addContour(offset, count, false);
}

void Lib2Tessellator::addHole(uint offset, size_t count) {
  addContour(offset, count, true);
}


GL::IndexVector Lib2Tessellator::triangulate() {
  if (!tessTesselate(m_tess, TESS_WINDING_POSITIVE, TESS_POLYGONS, 3, 2, nullptr)) {
    qWarning() << "Lib2Tessellation failed";
    return GL::IndexVector();
  }
  const int* elems = tessGetElements(m_tess);
  const int nelems = tessGetElementCount(m_tess);

  GL::IndexVector indices;
  const GLuint undef = TESS_UNDEF;
  for (int i = 0; i < nelems; i++)  {
    const GLuint i0 = elems[3 * i];
    const GLuint i1 = elems[3 * i + 1];
    const GLuint i2 = elems[3 * i + 2];
    if (i0 == undef || i1 == undef || i2 == undef) {
      qWarning() << "skipping Lib2 triangle";
      continue;
    }
    indices << m_indexCover[i0] << m_indexCover[i1] << m_indexCover[i2];
  }

  return indices;
}

