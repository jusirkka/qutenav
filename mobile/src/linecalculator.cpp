#include "linecalculator.h"
#include <QOpenGLExtraFunctions>
#include <glm/glm.hpp>

GL::LineCalculator* GL::LineCalculator::instance() {
  static LineCalculator* lc = new LineCalculator();
  return lc;
}

GL::LineCalculator::LineCalculator()
{}

GL::LineCalculator::~LineCalculator() {}

static const uint LEFT = 1;
static const uint RIGHT = 2;
static const uint BOTTOM = 4;
static const uint TOP = 8;

static uint locationCode(const glm::vec2& v, const QRectF& va) {
  uint code = 0;
  if (v.y > va.bottom()) {
    code |= TOP;
  } else if (v.y < va.top()) {
    code |= BOTTOM;
  }

  if (v.x > va.right()) {
    code |= RIGHT;
  } else if (v.x < va.left()) {
    code |= LEFT;
  }
  return code;
}

// Cohen-Sutherland test
bool outsideViewArea(const glm::vec2& v1, const glm::vec2& v2, const QRectF& va) {
  const uint c1 = locationCode(v1, va);
  const uint c2 = locationCode(v2, va);
  if (c1 == 0 && c2 == 0) return false;
  return (c1 & c2) != 0;
}


void GL::LineCalculator::calculate(VertexVector& transforms,
                                   GLfloat period,
                                   const QRectF& va,
                                   BufferData& vertices,
                                   BufferData& indices) {
  if (period < 1.e-10) {
    qWarning() << "GL::LineCalculator: Period is too small" << period;
    return;
  }

  vertices.buffer.bind();
  auto vertexBufferIn = reinterpret_cast<const glm::vec2*>(
        vertices.buffer.mapRange(0, vertices.count * sizeof(glm::vec2),
                                 QOpenGLBuffer::RangeRead));

  indices.buffer.bind();
  auto indexBufferIn = reinterpret_cast<const GLuint*>(
        indices.buffer.mapRange(0, indices.count * sizeof(GLuint),
                                 QOpenGLBuffer::RangeRead));

  const int numOutIndices = indices.count - 1;
  for (int index = 0; index < numOutIndices; index++) {
    const uint v1 = vertices.offset + indexBufferIn[indices.offset + index];
    const uint v2 = vertices.offset + indexBufferIn[indices.offset + index + 1];
    const glm::vec2 p1 = vertexBufferIn[v1];
    const glm::vec2 p2 = vertexBufferIn[v2];

    if (outsideViewArea(p1, p2, va)) {
      continue;
    }

    const float r = glm::length(p2 - p1);
    if (r < 1.e-10) {
      continue;
    }
    const glm::vec2 u = (p2 - p1) / r;
    float n = floor(r / period);
    const float r0 = period * (n + .5);
    if (r > r0 || n == 0) {
      n = n + 1;
    }
    const float s = r / (period * n);

    const glm::vec2 dir = s * period * u;
    for (int k = 0; k < n; k++) {
      const glm::vec2 v = p1 + (1.f * k) * dir;
      transforms << v.x << v.y << s * u.x << s * u.y;
    }
  }

  vertices.buffer.unmap();
  indices.buffer.unmap();
}



