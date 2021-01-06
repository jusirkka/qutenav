#include "s57object.h"
#include <QDate>
#include <QDebug>
#include "shader.h"
#include "s52presentation.h"
#include "platform.h"
#include "linecalculator.h"


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

bool S57::Object::canPaint(const QRectF& viewArea, quint32 scale,
                           const QDate& today, bool onlyViewArea) const {
  if (m_bbox.isValid() && !m_bbox.intersects(viewArea)) {
    // qDebug() << "no intersect" << m_bbox << viewArea << name();
    return false;
  }

  if (onlyViewArea) return true;

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


bool S57::Object::canPaint(quint32 scale) const {

  if (m_attributes.contains(scaminIndex)) {
    const quint32 mx = m_attributes[scaminIndex].value().toUInt();
    if (scale > mx) {
      // qDebug() << "scale too small" << scale << mx << name();
      return false;
    }
  }

  return true;
}

//
// Paintdata
//

S57::PaintData::PaintData(Type t)
  : m_type(t)
{}

S57::OverrideData::OverrideData(bool uw)
  : PaintData(Type::Override)
  , m_underwater(uw)
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

S57::LineData::LineData(Type t, const ElementDataVector& elems, GLfloat lw)
  : PaintData(t)
  , m_elements(elems)
  , m_lineWidth(lw)
  , m_conv(1. / S52::LineWidthDots(1))
{}

void S57::SolidLineData::setUniforms() const {
  auto prog = GL::SolidLineShader::instance();
  prog->prog()->setUniformValue(prog->m_locations.base_color, m_color);
  prog->prog()->setUniformValue(prog->m_locations.lineWidth, m_conv * m_lineWidth);
}

void S57::SolidLineData::setVertexOffset() const {
  auto prog = GL::SolidLineShader::instance()->prog();
  prog->setAttributeBuffer(0, GL_FLOAT, m_vertexOffset, 2, 0);
}


S57::SolidLineData::SolidLineData(Type t,
                                  const ElementDataVector& elems,
                                  GLsizei offset,
                                  const QColor& c,
                                  GLfloat width)
  : LineData(t, elems, width)
  , m_vertexOffset(offset)
  , m_color(c)
{}


void S57::DashedLineData::setUniforms() const {
  auto prog = GL::DashedLineShader::instance();
  prog->prog()->setUniformValue(prog->m_locations.base_color, m_color);
  prog->prog()->setUniformValue(prog->m_locations.lineWidth, m_conv * m_lineWidth);
  prog->prog()->setUniformValue(prog->m_locations.pattern, m_pattern);
}

void S57::DashedLineData::setVertexOffset() const {
  auto prog = GL::DashedLineShader::instance()->prog();
  prog->setAttributeBuffer(0, GL_FLOAT, m_vertexOffset, 2, 0);
}


S57::DashedLineData::DashedLineData(Type t, const
                                    ElementDataVector& elems,
                                    GLsizei offset,
                                    const QColor& c,
                                    GLfloat width,
                                    uint patt)
  : LineData(t, elems, width)
  , m_vertexOffset(offset)
  , m_color(c)
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

S57::SolidLineLocalData::SolidLineLocalData(const GL::VertexVector& vertices,
                                            const ElementDataVector& elem,
                                            const QColor& c,
                                            GLfloat width,
                                            bool dispU,
                                            const QPointF& p)
  : SolidLineData(Type::SolidLineLocal, elem, 0, c, width)
  , m_vertices(vertices)
  , m_displayUnits(dispU)
  , m_pivot(p)
{}

S57::PaintData* S57::SolidLineLocalData::globalize(GLsizei offset) const {
  return new SolidLineArrayData(m_elements, offset, m_color, m_lineWidth);
}

GL::VertexVector S57::SolidLineLocalData::vertices(qreal scale) {
  if (m_displayUnits) {
    GL::VertexVector vs;
    scale *= 2 / dots_per_mm_y;
    for (int i = 0; i < m_vertices.size() / 2; i++) {
      const QPointF p1(m_vertices[2 * i], m_vertices[2 * i + 1]);
      const QPointF p = m_pivot + (p1 - m_pivot) / scale;
      vs << p.x() << p.y();
    }
    return vs;
  }
  return m_vertices;
}

S57::DashedLineLocalData::DashedLineLocalData(const GL::VertexVector& vertices,
                                              const ElementDataVector& elem,
                                              const QColor& c,
                                              GLfloat width,
                                              uint pattern,
                                              bool dispU,
                                              const QPointF& p)
  : DashedLineData(Type::DashedLineLocal, elem, 0, c, width, pattern)
  , m_vertices(vertices)
  , m_displayUnits(dispU)
  , m_pivot(p)
{}

S57::PaintData* S57::DashedLineLocalData::globalize(GLsizei offset) const {
  return new DashedLineArrayData(m_elements, offset, m_color, m_lineWidth, m_pattern);
}

GL::VertexVector S57::DashedLineLocalData::vertices(qreal scale) {
  if (m_displayUnits) {
    GL::VertexVector vs;
    scale *= 2 / dots_per_mm_y;
    for (int i = 0; i < m_vertices.size() / 2; i++) {
      const QPointF p1(m_vertices[2 * i], m_vertices[2 * i + 1]);
      const QPointF p = m_pivot + (p1 - m_pivot) / scale;
      vs << p.x() << p.y();
    }
    return vs;
  }
  return m_vertices;
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


void S57::RasterHelper::setSymbolOffset(const QPoint &off) const {
  auto sh = GL::RasterSymbolShader::instance();
  sh->prog()->setUniformValue(sh->m_locations.offset, off);
}

void S57::RasterHelper::setVertexBufferOffset(GLsizei off) const {
  auto prog = GL::RasterSymbolShader::instance()->prog();
  prog->setAttributeBuffer(2, GL_FLOAT, off, 2, 0);
}

void S57::RasterHelper::setColor(const QColor&) const {
  // noop
}

void S57::VectorHelper::setSymbolOffset(const QPoint &off) const {
  // noop
}

void S57::VectorHelper::setVertexBufferOffset(GLsizei off) const {
  auto prog = GL::VectorSymbolShader::instance()->prog();
  prog->setAttributeBuffer(1, GL_FLOAT, off, 4, 0);
}

void S57::VectorHelper::setColor(const QColor& c) const {
  auto sh = GL::VectorSymbolShader::instance();
  sh->prog()->setUniformValue(sh->m_locations.base_color, c);
}


S57::SymbolPaintDataBase::SymbolPaintDataBase(Type t,
                                              S52::SymbolType s,
                                              quint32 index,
                                              const QPoint& offset,
                                              SymbolHelper* helper)
  : PaintData(t)
  , m_type(s)
  , m_index(index)
  , m_offset(offset)
  , m_helper(helper)
  , m_pivotOffset(0)
  , m_pivots()
  , m_instanceCount(1)
{}

S57::SymbolPaintDataBase::~SymbolPaintDataBase() {
  delete m_helper;
}

void S57::SymbolPaintDataBase::getPivots(GL::VertexVector& pivots) {
  m_pivotOffset = pivots.size() * sizeof(GLfloat);
  pivots.append(m_pivots);
  m_pivots.clear();
}

void S57::SymbolPaintDataBase::setUniforms() const {
  m_helper->setSymbolOffset(m_offset);
}

void S57::SymbolPaintDataBase::setVertexOffset() const {
  m_helper->setVertexBufferOffset(m_pivotOffset);
}


S57::SymbolPaintData::SymbolPaintData(Type t,
                                      quint32 index,
                                      const QPoint& offset,
                                      SymbolHelper* helper,
                                      const QPointF& pivot)
  : SymbolPaintDataBase(t, S52::SymbolType::Single, index, offset, helper)
{
  m_pivots << pivot.x() << pivot.y();
}

S57::RasterSymbolPaintData::RasterSymbolPaintData(quint32 index,
                                                  const QPoint& offset,
                                                  const QPointF& pivot,
                                                  const ElementData& elem)
  : SymbolPaintData(Type::RasterSymbols, index, offset, new RasterHelper(), pivot)
  , m_elem(elem)
{}

void S57::RasterSymbolPaintData::merge(const SymbolPaintDataBase* other, qreal, const QRectF&) {
  if (other == nullptr) return;
  auto r = dynamic_cast<const RasterSymbolPaintData*>(other);
  Q_ASSERT(r->m_pivots.size() == 2);
  m_pivots.append(r->m_pivots);
  m_instanceCount += 1;
}

S57::VectorSymbolPaintData::VectorSymbolPaintData(quint32 index,
                                                  const QPointF& pivot,
                                                  const Angle& rot,
                                                  const QT::ColorVector& colors,
                                                  const ElementDataVector& elems)
  : SymbolPaintData(Type::VectorSymbols, index, QPoint(), new VectorHelper(), pivot)
{
  m_pivots << rot.cos() << rot.sin();
  for (int i = 0; i < colors.size(); i++) {
    ColorElementData d;
    d.color = colors[i];
    d.element = elems[i];
    m_elems.append(d);
  }
}

void S57::VectorSymbolPaintData::merge(const SymbolPaintDataBase* other, qreal, const QRectF&) {
  if (other == nullptr) return;
  auto s = dynamic_cast<const VectorSymbolPaintData*>(other);
  Q_ASSERT(s->m_pivots.size() == 4);
  m_pivots.append(s->m_pivots);
  m_instanceCount += 1;
}


void S57::VectorSymbolPaintData::setColor(const QColor& c) const {
  m_helper->setColor(c);
}


S57::PatternPaintData::PatternPaintData(Type t,
                                        quint32 index,
                                        const QPoint& offset,
                                        SymbolHelper* helper,
                                        const ElementDataVector& aelems,
                                        GLsizei aoffset,
                                        bool indexed,
                                        const QRectF& bbox,
                                        const PatternAdvance& advance)
  : SymbolPaintDataBase(t, S52::SymbolType::Pattern, index, offset, helper)
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


void S57::PatternPaintData::setAreaVertexOffset(GLsizei off) const {
  auto prog = GL::AreaShader::instance()->prog();
  prog->setAttributeBuffer(0, GL_FLOAT, off, 2, 0);
}

void S57::PatternPaintData::merge(const SymbolPaintDataBase *other, qreal scale, const QRectF& va) {
  if (other == nullptr) {
    Q_ASSERT(m_areaElements.size() + m_areaArrays.size() == 1);
    for (const AreaData& a: m_areaElements) {
      createPivots(a.bbox.intersected(va), scale);
    }
    for (const AreaData& a: m_areaArrays) {
      createPivots(a.bbox.intersected(va), scale);
    }
  } else {
    auto r = dynamic_cast<const PatternPaintData*>(other);

    Q_ASSERT(r->m_areaElements.size() + r->m_areaArrays.size() == 1);
    for (const AreaData& a: r->m_areaElements) {
      createPivots(a.bbox.intersected(va), scale);
    }
    for (const AreaData& a: r->m_areaArrays) {
      createPivots(a.bbox.intersected(va), scale);
    }
    m_areaArrays.append(r->m_areaArrays);
    m_areaElements.append(r->m_areaElements);
  }
  m_instanceCount = m_pivots.size() / 2;
}


S57::RasterPatternPaintData::RasterPatternPaintData(quint32 index,
                                                    const QPoint& offset,
                                                    const ElementDataVector& aelems,
                                                    GLsizei aoffset,
                                                    bool indexed,
                                                    const QRectF& bbox,
                                                    const PatternAdvance& advance,
                                                    const ElementData& elem)
  : PatternPaintData(Type::RasterPatterns, index, offset, new RasterHelper(),
                     aelems, aoffset, indexed, bbox, advance)
  , m_elem(elem)
{}



void S57::RasterPatternPaintData::createPivots(const QRectF& bbox, qreal scale) {
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


S57::VectorPatternPaintData::VectorPatternPaintData(quint32 index,
                                                    const ElementDataVector& aelems,
                                                    GLsizei aoffset,
                                                    bool indexed,
                                                    const QRectF& bbox,
                                                    const PatternAdvance& advance,
                                                    const Angle& rot,
                                                    const QT::ColorVector& colors,
                                                    const ElementDataVector& elems)
  : PatternPaintData(Type::VectorPatterns, index, QPoint(), new VectorHelper(),
                     aelems, aoffset, indexed, bbox, advance)
{
  m_c = rot.cos();
  m_s = rot.sin();
  for (int i = 0; i < colors.size(); i++) {
    ColorElementData d;
    d.color = colors[i];
    d.element = elems[i];
    m_elems.append(d);
  }
}

void S57::VectorPatternPaintData::setColor(const QColor& c) const {
  m_helper->setColor(c);
}

void S57::VectorPatternPaintData::createPivots(const QRectF& bbox, qreal scale) {

  scale *= 200 / dots_per_mm_y;

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
      m_pivots << kx * X + x1 << ky * Y << m_c << m_s;
    }
  }
}


S57::LineStylePaintData::LineStylePaintData(quint32 index,
                                            const ElementDataVector& lelems,
                                            GLsizei loffset,
                                            const QRectF& bbox,
                                            const PatternAdvance& advance,
                                            const QT::ColorVector& colors,
                                            const ElementDataVector& elems)
  : SymbolPaintDataBase(Type::VectorLineStyles, S52::SymbolType::LineStyle, index, QPoint(), new VectorHelper)
  , m_lineElements()
  , m_advance(advance.x)
  , m_viewArea()
{
  LineData d;
  d.elements = lelems;
  d.vertexOffset = loffset;
  d.bbox = bbox;
  m_lineElements.append(d);

  for (int i = 0; i < colors.size(); i++) {
    ColorElementData c;
    c.color = colors[i];
    c.element = elems[i];
    m_elems.append(c);
  }
}

void S57::LineStylePaintData::merge(const SymbolPaintDataBase* other, qreal scale, const QRectF& va) {
  if (other == nullptr) {
    scale *= 200 / dots_per_mm_y;
    m_advance /= scale;
    m_viewArea = va;
    Q_ASSERT(m_lineElements.size() == 1);
  } else {
    auto r = dynamic_cast<const LineStylePaintData*>(other);
    Q_ASSERT(r->m_lineElements.size() == 1);
    m_lineElements.append(r->m_lineElements);
  }
}

void S57::LineStylePaintData::createTransforms(GL::VertexVector& transforms,
                                               const QOpenGLBuffer& coordBuffer,
                                               const QOpenGLBuffer& indexBuffer,
                                               GLsizei maxCoordOffset) {
  m_pivotOffset = transforms.size() * sizeof(GLfloat);

  auto lc = GL::LineCalculator::instance();
  using BufferData = GL::LineCalculator::BufferData;
  BufferData vs;
  vs.buffer = coordBuffer;
  BufferData is;
  is.buffer = indexBuffer;
  for (LineData& d: m_lineElements) {
    vs.offset = d.vertexOffset / sizeof(GLfloat) / 2;
    vs.count = (maxCoordOffset - d.vertexOffset) / sizeof(GLfloat) / 2;
    const QRectF va = d.bbox.intersected(m_viewArea);
    for (S57::ElementData elem: d.elements) {
      // account adjacency
      is.offset = elem.offset / sizeof(GLuint) + 1;
      is.count = elem.count - 2;
      lc->calculate(transforms, m_advance, va, vs, is);
    }
  }

  m_instanceCount = (transforms.size() - m_pivotOffset / sizeof(GLfloat)) / 4;
}

void S57::LineStylePaintData::setColor(const QColor& c) const {
  m_helper->setColor(c);
}


