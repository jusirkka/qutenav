#include "s57object.h"
#include <QDate>
#include <QDebug>
#include "shader.h"
#include "s52presentation.h"
#include "platform.h"



bool S57::Attribute::matches(const Attribute &constraint) const {
  if (constraint.type() == Type::Any) {
    if (m_type == Type::None) return false;
    return true;
  }
  if (constraint.type() == Type::None) {
    if (m_type == Type::None) return true;
    return false;
  }
  if (m_type != constraint.type()) return false;

  if (m_type == Type::Real) {
    return std::abs(m_value.toDouble() - constraint.value().toDouble()) < 1.e-6;
  }

  // Compare integer lists: Note that we require exact match
  // not just up to constraint list size
  if (m_type == Type::IntegerList) {
    QVariantList clist = constraint.value().toList();
    QVariantList mlist = m_value.toList();
    if (mlist.size() != clist.size()) return false;
    for (int i = 0; i < clist.size(); i++) {
      if (mlist[i].toInt() != clist[i].toInt()) return false;
    }
    return true;
  }

  return m_value == constraint.value();
}

static QDate stringToDate(const QString& s, const QDate& today, bool start) {
  if (s.length() == 8) return QDate::fromString(s, "yyyyMMdd");

  if (s.length() == 4) {
    auto d = QDate::fromString(s, "MMdd");
    d.setDate(today.year(), d.month(), d.day());
    return d;
  }

  if (s.length() == 2) {
    auto d = QDate::fromString(s, "MM");
    if (start) {
      d.setDate(today.year(), d.month(), 1);
    } else {
      int day = 31;
      const int month = d.month();
      while (!d.setDate(today.year(), month, day) && day >= 28) {
        day -= 1;
      }
    }
    return d;
  }

  return QDate();
}

S57::Object::~Object() {
  delete m_geometry;
}

QString S57::Object::name() const {
  auto s = QString("%1-%2-%3").arg(identifier()).arg(classCode()).arg(char(geometry()->type()));
  return s;
}

QVariant S57::Object::attributeValue(quint32 attr) const {
  if (!m_attributes.contains(attr)) {
    // qWarning() << "[Attribute]" << S52::GetAttributeName(attr) << "not present [" << m_geometry->centerLL().print() << "]";
    return QVariant();
  }
  return m_attributes[attr].value();
}

bool S57::Object::canPaint(const QRectF& viewArea, quint32 scale, const QDate& today) const {
  if (m_bbox.isValid() && !m_bbox.intersects(viewArea)) {
    // qDebug() << "no intersect" << m_bbox << viewArea << name();
    return false;
  }

  if (m_attributes.contains(scaminIndex)) {
    const quint32 mx = m_attributes[scaminIndex].value().toUInt();
    if (scale > mx) {
      // qDebug() << "scale too small" << scale << mx << name();
      return false;
    }
  }

  if (m_attributes.contains(datstaIndex)) {
    auto d = stringToDate(m_attributes[datstaIndex].value().toString(), today, true);
    if (today < d) return false;
  }

  if (m_attributes.contains(datendIndex)) {
    auto d = stringToDate(m_attributes[datendIndex].value().toString(), today, false);
    if (today > d) return false;
  }

  if (m_attributes.contains(perstaIndex)) {
    auto d = stringToDate(m_attributes[perstaIndex].value().toString(), today, true);
    if (today < d) return false;
  }

  if (m_attributes.contains(perendIndex)) {
    auto d = stringToDate(m_attributes[perendIndex].value().toString(), today, false);
    if (today > d) return false;
  }

  return true;
}

//
// Paintdata
//

S57::PaintData::PaintData(Type t)
  : m_type(t)
{}


void S57::TriangleData::setUniforms() const {
  auto prog = GL::AreaShader::instance();
  prog->prog()->setUniformValue(prog->m_locations.base_color, m_color);
}

void S57::TriangleData::setVertexOffset() const {
  auto prog = GL::AreaShader::instance()->prog();
  prog->setAttributeBuffer(0, GL_FLOAT, m_vertexOffset, 2, 0);
}

S57::TriangleData::TriangleData(Type t, const ElementDataVector& elems, GLsizei offset, const QColor& c)
  : PaintData(t)
  , m_elements(elems)
  , m_vertexOffset(offset)
  , m_color(c)
{}

S57::TriangleArrayData::TriangleArrayData(const ElementDataVector& elem, GLsizei offset, const QColor& c)
  : TriangleData(Type::TriangleArrays, elem, offset, c)
{}

S57::TriangleElemData::TriangleElemData(const ElementDataVector& elem, GLsizei offset, const QColor& c)
  : TriangleData(Type::TriangleElements, elem, offset, c)
{}

S57::LineData::LineData(Type t, const ElementDataVector& elems)
  : PaintData(t)
  , m_elements(elems)
{}

void S57::SolidLineData::setUniforms() const {
  auto prog = GL::SolidLineShader::instance();
  prog->prog()->setUniformValue(prog->m_locations.base_color, m_color);
  // FIXME too thick lines without correction factor
  prog->prog()->setUniformValue(prog->m_locations.lineWidth, .5f * m_lineWidth);
}

void S57::SolidLineData::setVertexOffset() const {
  auto prog = GL::SolidLineShader::instance()->prog();
  prog->setAttributeBuffer(0, GL_FLOAT, m_vertexOffset, 2, 0);
}


S57::SolidLineData::SolidLineData(Type t, const ElementDataVector& elems, GLsizei offset, const QColor& c, GLfloat width)
  : LineData(t, elems)
  , m_vertexOffset(offset)
  , m_color(c)
  , m_lineWidth(width)
{}


void S57::DashedLineData::setUniforms() const {
  auto prog = GL::DashedLineShader::instance();
  prog->prog()->setUniformValue(prog->m_locations.base_color, m_color);
  // FIXME too thick lines without correction factor
  prog->prog()->setUniformValue(prog->m_locations.lineWidth, .5f * m_lineWidth);
  prog->prog()->setUniformValue(prog->m_locations.pattern, m_pattern);
}

void S57::DashedLineData::setVertexOffset() const {
  auto prog = GL::DashedLineShader::instance()->prog();
  prog->setAttributeBuffer(0, GL_FLOAT, m_vertexOffset, 2, 0);
}


S57::DashedLineData::DashedLineData(Type t, const ElementDataVector& elems, GLsizei offset, const QColor& c, GLfloat width, uint patt)
  : LineData(t, elems)
  , m_vertexOffset(offset)
  , m_color(c)
  , m_lineWidth(width)
  , m_pattern(patt)
{}


S57::SolidLineElemData::SolidLineElemData(const ElementDataVector& elem, GLsizei offset, const QColor& c, GLfloat width)
  : SolidLineData(Type::SolidLineElements, elem, offset, c, width)
{}

S57::SolidLineArrayData::SolidLineArrayData(const ElementDataVector& elem, GLsizei offset, const QColor& c, GLfloat width)
  : SolidLineData(Type::SolidLineArrays, elem, offset, c, width)
{}

S57::DashedLineElemData::DashedLineElemData(const ElementDataVector& elem, GLsizei offset, const QColor& c, GLfloat width, uint pattern)
  : DashedLineData(Type::DashedLineElements, elem, offset, c, width, pattern)
{}

S57::DashedLineArrayData::DashedLineArrayData(const ElementDataVector& elem, GLsizei offset, const QColor& c, GLfloat width, uint pattern)
  : DashedLineData(Type::DashedLineArrays, elem, offset, c, width, pattern)
{}

S57::SolidLineLocalData::SolidLineLocalData(const VertexVector& vertices, const ElementDataVector& elem, const QColor& c, GLfloat width)
  : SolidLineData(Type::SolidLineLocal, elem, 0, c, width)
  , m_vertices(vertices)
{}

S57::PaintData* S57::SolidLineLocalData::globalize(GLsizei offset) const {
  return new SolidLineArrayData(m_elements, offset, m_color, m_lineWidth);
}


S57::DashedLineLocalData::DashedLineLocalData(const VertexVector& vertices, const ElementDataVector& elem, const QColor& c, GLfloat width, uint pattern)
  : DashedLineData(Type::DashedLineLocal, elem, 0, c, width, pattern)
  , m_vertices(vertices)
{}

S57::PaintData* S57::DashedLineLocalData::globalize(GLsizei offset) const {
  return new DashedLineArrayData(m_elements, offset, m_color, m_lineWidth, m_pattern);
}


S57::TextElemData::TextElemData(const QPointF& pivot,
                                const QPointF& bboxBase,
                                const QPointF& pivotOffset,
                                const QPointF& bboxOffset,
                                float boxScale,
                                GLsizei vertexOffset,
                                const ElementData& elems,
                                const QColor& c)
  : PaintData(Type::TextElements)
  , m_elements(elems)
  , m_vertexOffset(vertexOffset)
  , m_color(c)
  , m_scaleMM(boxScale)
  , m_pivot(pivot)
  , m_shiftMM(bboxOffset + boxScale * (pivotOffset - bboxBase))
{}


void S57::TextElemData::setUniforms() const {
  auto prog = GL::TextShader::instance();
  prog->prog()->setUniformValue(prog->m_locations.base_color, m_color);
  prog->prog()->setUniformValue(prog->m_locations.textScale, static_cast<GLfloat>(m_scaleMM * dots_per_mm_y));
  prog->prog()->setUniformValue(prog->m_locations.pivot, m_pivot);
  prog->prog()->setUniformValue(prog->m_locations.offset, m_shiftMM * dots_per_mm_y);
}

void S57::TextElemData::setVertexOffset() const {
  auto prog = GL::TextShader::instance()->prog();
  const int texOffset = 2 * sizeof(GLfloat);
  const int stride = 4 * sizeof(GLfloat);
  prog->setAttributeBuffer(0, GL_FLOAT, m_vertexOffset, 2, stride);
  prog->setAttributeBuffer(1, GL_FLOAT, m_vertexOffset + texOffset, 2, stride);
}


S57::RasterData::RasterData(Type t,
                            const QPoint& offset,
                            const ElementData& elems,
                            quint32 index)
  : PaintData(t)
  , m_offset(offset)
  , m_elements(elems)
  , m_index(index)
  , m_instanceCount(1)
  , m_pivots()
{
}

void S57::RasterData::getPivots(PivotVector &pivots) {
  m_pivotOffset = pivots.size() * sizeof(GLfloat);
  pivots.append(m_pivots);
  m_pivots.clear();
}

void S57::RasterData::setUniforms() const {
  auto prog = GL::RasterSymbolShader::instance();
  prog->prog()->setUniformValue(prog->m_locations.offset, m_offset);
}

void S57::RasterData::setVertexOffset() const {
  auto prog = GL::RasterSymbolShader::instance()->prog();
  prog->setAttributeBuffer(2, GL_FLOAT, m_pivotOffset, 2, 0);
}

S57::RasterSymbolData::RasterSymbolData(const QPointF &pivot,
                                        const QPoint &offset,
                                        const ElementData &elems,
                                        quint32 index)
  : RasterData(Type::RasterSymbolElements, offset, elems, index)
{
  m_pivots << pivot.x() << pivot.y();
}

SymbolKey S57::RasterSymbolData::key() const {
  return SymbolKey(m_index, S52::SymbolType::Single);
}

void S57::RasterSymbolData::merge(const SymbolMerger *other, qreal) {
  if (other == nullptr) return;
  auto r = dynamic_cast<const RasterSymbolData*>(other);
  Q_ASSERT(r->m_pivots.size() == 2);
  m_pivots.append(r->m_pivots);
  m_instanceCount += 1;
}

S57::RasterPatternData::RasterPatternData(const ElementDataVector& aelems,
                                          GLsizei aoffset,
                                          bool indexed,
                                          const QRectF& bbox,
                                          const QPoint& offset,
                                          const PatternAdvance& advance,
                                          const ElementData& elem,
                                          quint32 index)
  : RasterData(Type::RasterPatterns, offset, elem, index)
  , m_areaElements()
  , m_areaArrays()
  , m_advance(advance)
{
  AreaData d;
  d.elements = aelems;
  d.vertexOffset = aoffset;
  d.bbox = bbox;
  if (indexed) {
    m_areaElements.append(d);
  } else {
    m_areaArrays.append(d);
  }
}

SymbolKey S57::RasterPatternData::key() const {
  return SymbolKey(m_index, S52::SymbolType::Pattern);
}

void S57::RasterPatternData::merge(const SymbolMerger *other, qreal scale) {
  if (other == nullptr) {
    Q_ASSERT(m_areaElements.size() + m_areaArrays.size() == 1);
    for (const AreaData& a: m_areaElements) {
      createPivots(a.bbox, scale);
    }
    for (const AreaData& a: m_areaArrays) {
      createPivots(a.bbox, scale);
    }
  } else {
    auto r = dynamic_cast<const RasterPatternData*>(other);

    Q_ASSERT(r->m_areaElements.size() + r->m_areaArrays.size() == 1);
    for (const AreaData& a: r->m_areaElements) {
      createPivots(a.bbox, scale);
    }
    for (const AreaData& a: r->m_areaArrays) {
      createPivots(a.bbox, scale);
    }
    m_areaArrays.append(r->m_areaArrays);
    m_areaElements.append(r->m_areaElements);
  }
  m_instanceCount = m_pivots.size() / 2;
}

void S57::RasterPatternData::createPivots(const QRectF &bbox, qreal scale) {
  const qreal X = m_advance.x / scale;
  const qreal Y = m_advance.xy.y() / scale;
  const qreal xs = m_advance.xy.x() / scale;

  const int ny = std::floor(bbox.top() / Y);
  const int my = std::ceil(bbox.bottom() / Y) + 1;

  for (int ky = ny; ky < my; ky++) {
    const qreal x1 = ky % 2 == 0 ? 0. : xs;
    const int nx = std::floor((bbox.left() - x1) / X);
    const int mx = std::ceil((bbox.right() - x1) / X) + 1;
    for (int kx = nx; kx < mx; kx++) {
      m_pivots << kx * X + x1 << ky * Y;
    }
  }
}

void S57::RasterPatternData::setAreaVertexOffset(GLsizei off) const {
  auto prog = GL::AreaShader::instance()->prog();
  prog->setAttributeBuffer(0, GL_FLOAT, off, 2, 0);
}

