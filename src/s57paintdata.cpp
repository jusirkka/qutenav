/* -*- coding: utf-8-unix -*-
 *
 * File: src/s57paintdata.cpp
 *
 * Copyright (C) 2021 Jukka Sirkka
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "s57paintdata.h"
#include "shader.h"
#include <QOpenGLExtraFunctions>
#include "linecalculator.h"
#include "settings.h"
#include "region.h"
#include "textmanager.h"
#include "gnuplot.h"

//
// Paintdata
//

S57::PaintData::PaintData(Type t)
  : m_type(t)
{}

S57::OverrideData::OverrideData(bool ovr)
  : PaintData(Type::Override)
  , m_override(ovr)
{}

S57::PriorityData::PriorityData(int prio)
  : PaintData(Type::Priority)
  , m_priority(prio)
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

static void filterElems(S57::ElementDataVector& elems, const KV::Region& cover) {

  // tag & copy: presumably faster than erasing in middle vector

  QVector<int> indices;
  for (int i = 0; i < elems.size(); ++i) {
    if (cover.intersects(elems[i].bbox)) {
      indices << i;
    }
  }

  if (indices.size() != elems.size()) {
    // qDebug() << "Filtered" << elems.size() << "->" << indices.size();
    S57::ElementDataVector orig = elems;
    elems.clear();
    for (int i: indices) {
      elems << orig[i];
    }
  }
}

void S57::TriangleData::filterElements(const KV::Region& cover) {
  filterElems(m_elements, cover);
}

S57::TriangleArrayData::TriangleArrayData(const ElementDataVector& elem, GLsizei offset, const QColor& c)
  : TriangleData(Type::TriangleArrays, elem, offset, c)
{
  if (c.alpha() < 255) {
    m_type = Type::LucentTriangleArrays;
  }
}

S57::TriangleElemData::TriangleElemData(const ElementDataVector& elem, GLsizei offset, const QColor& c)
  : TriangleData(Type::TriangleElements, elem, offset, c)
{
  if (c.alpha() < 255) {
    m_type = Type::LucentTriangleElements;
  }
}


S57::LineData::LineData(Type t,
                        const ElementDataVector& elems,
                        GLsizei offset,
                        const QColor& c,
                        GLfloat lw,
                        uint patt)
  : PaintData(t)
  , m_elements(elems)
  , m_lineWidth(lw)
  , m_vertexOffset(offset)
  , m_color(c)
  , m_pattern(patt)
{}

void S57::LineData::filterElements(const KV::Region& cover) {
  filterElems(m_elements, cover);
}


void S57::LineData::setVertexOffset() const {
  // noop
}


S57::LineElemData::LineElemData(const ElementDataVector& elem,
                                GLsizei offset,
                                const QColor& c,
                                GLfloat width,
                                uint pattern)
  : LineData(Type::LineElements, elem, offset, c, width, pattern)
{
  if (pattern != as_numeric(S52::LineType::Solid) || c.alpha() < 255) {
    m_type = Type::LucentLineElements;
  }
}

void S57::LineElemData::setUniforms() const {
  auto prog = GL::LineElemShader::instance();
  prog->prog()->setUniformValue(prog->m_locations.base_color, m_color);
  const float dw = Settings::instance()->displayLineWidthScaling();
  prog->prog()->setUniformValue(prog->m_locations.lineWidth, m_lineWidth * dw);
  auto f = QOpenGLContext::currentContext()->extraFunctions();
  f->glUniform1ui(prog->m_locations.pattern, m_pattern);
}


void S57::LineElemData::setStorageOffsets(uintptr_t offset) const {
  auto prog = GL::LineElemShader::instance();
  auto f = QOpenGLContext::currentContext()->extraFunctions();
  f->glUniform1ui(prog->m_locations.vertexOffset,
                  static_cast<GLuint>(m_vertexOffset / 2 / sizeof(GLfloat)));
  f->glUniform1ui(prog->m_locations.indexOffset,
                  static_cast<GLuint>(offset / sizeof(GLuint)));
}

S57::LineArrayData::LineArrayData(const ElementDataVector& elem,
                                  GLsizei offset,
                                  const QColor& c,
                                  GLfloat width,
                                  uint pattern)
  : LineData(Type::LineArrays, elem, offset, c, width, pattern)
{
  if (pattern != as_numeric(S52::LineType::Solid) || c.alpha() < 255) {
    m_type = Type::LucentLineArrays;
  }
}

void S57::LineArrayData::setUniforms() const {
  auto prog = GL::LineArrayShader::instance();
  prog->prog()->setUniformValue(prog->m_locations.base_color, m_color);
  const float dw = Settings::instance()->displayLineWidthScaling();
  prog->prog()->setUniformValue(prog->m_locations.lineWidth, m_lineWidth * dw);

  auto f = QOpenGLContext::currentContext()->extraFunctions();
  f->glUniform1ui(prog->m_locations.pattern, m_pattern);
}

void S57::LineArrayData::setStorageOffsets(uintptr_t offset) const {
  auto prog = GL::LineArrayShader::instance();
  auto f = QOpenGLContext::currentContext()->extraFunctions();
  f->glUniform1ui(prog->m_locations.vertexOffset,
                  static_cast<GLuint>(m_vertexOffset / 2 / sizeof(GLfloat) + offset));
}

S57::SegmentArrayData::SegmentArrayData(int count,
                                        GLsizei offset,
                                        const QColor& c,
                                        GLfloat width,
                                        uint pattern)
  : LineData(Type::LineArrays, {ElementData(count)}, offset, c, width, pattern)
{}

void S57::SegmentArrayData::setUniforms() const {
  auto prog = GL::SegmentArrayShader::instance();
  prog->prog()->setUniformValue(prog->m_locations.base_color, m_color);
  const float dw = Settings::instance()->displayLineWidthScaling();
  prog->prog()->setUniformValue(prog->m_locations.lineWidth, m_lineWidth * dw);

  auto f = QOpenGLContext::currentContext()->extraFunctions();
  f->glUniform1ui(prog->m_locations.pattern, m_pattern);
}

void S57::SegmentArrayData::setStorageOffsets(uintptr_t offset) const {
  auto prog = GL::SegmentArrayShader::instance();
  auto f = QOpenGLContext::currentContext()->extraFunctions();
  f->glUniform1ui(prog->m_locations.vertexOffset,
                  static_cast<GLuint>(m_vertexOffset / 2 / sizeof(GLfloat) + offset));
}

S57::LineLocalData::LineLocalData(const GL::VertexVector& vertices,
                                  const ElementDataVector& elem,
                                  const QColor& c,
                                  GLfloat width,
                                  uint pattern,
                                  bool dispU,
                                  const QPointF& p)
  : LineData(Type::LineLocal, elem, 0, c, width, pattern)
  , m_vertices(vertices)
  , m_displayUnits(dispU)
  , m_pivot(p)
{}

void S57::LineLocalData::setUniforms() const {
  // noop
}

void S57::LineLocalData::setStorageOffsets(uintptr_t) const {
  // noop
}



S57::PaintData* S57::LineLocalData::globalize(GLsizei offset, qreal scale) const {
  if (m_displayUnits) {
    // transform millimeters (display) to meters (chart) in bounding boxes
    // first scale nominal millimeters to actual millimeters
    scale *= Settings::instance()->displayLengthScaling();
    ElementDataVector elems;
    for (const ElementData& elem: m_elements) {
      ElementData e;
      e.mode = elem.mode; e.count = elem.count; e.offset = elem.offset;

      const QPointF tl = m_pivot + (elem.bbox.topLeft() - m_pivot) / scale;
      const QPointF br = m_pivot + (elem.bbox.bottomRight() - m_pivot) / scale;
      e.bbox = QRectF(tl, br);

      elems << e;
    }
    return new LineArrayData(elems, offset, m_color, m_lineWidth, m_pattern);
  }
  return new LineArrayData(m_elements, offset, m_color, m_lineWidth, m_pattern);
}

GL::VertexVector S57::LineLocalData::vertices(qreal scale) {
  if (m_displayUnits) {
    // transform millimeters (display) to meters (chart)
    // first scale nominal millimeters to actual millimeters
    scale *= Settings::instance()->displayLengthScaling();
    GL::VertexVector vs;
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
                                int ticket,
                                const QColor& c)
  : PaintData(Type::TextElements)
  , m_pivots {pivot}
  , m_tickets {ticket}
  , m_color(c)
  , m_instanceCount(0)
{}


void S57::TextElemData::setUniforms() const {
  auto prog = GL::TextShader::instance();
  prog->prog()->setUniformValue(prog->m_locations.base_color, m_color);
}

void S57::TextElemData::setVertexOffset() const {
  auto prog = GL::TextShader::instance()->prog();
  // vertex x 4, tex x 4, pivot x2 = 10
  const int stride = 10 * sizeof(GLfloat);
  const int texOffset = 4 * sizeof(GLfloat);
  const int pivotOffset = 8 * sizeof(GLfloat);
  prog->setAttributeBuffer(0, GL_FLOAT, m_instanceOffset, 4, stride);
  prog->setAttributeBuffer(1, GL_FLOAT, m_instanceOffset + texOffset, 4, stride);
  prog->setAttributeBuffer(2, GL_FLOAT, m_instanceOffset + pivotOffset, 2, stride);
}

void S57::TextElemData::merge(const TextElemData* other) {
  Q_ASSERT(other->m_tickets.size() == 1);
  m_tickets << other->m_tickets.first();
  m_pivots << other->m_pivots.first();
}

void S57::TextElemData::getInstances(GL::VertexVector& instances) {
  m_instanceOffset = instances.size() * sizeof(GLfloat);
  for (int ticket: m_tickets) {
    const auto pivot = m_pivots.first();
    GL::VertexVector vs = TextManager::instance()->shapeData(ticket);
    const int numShapes = vs.size() / 10;
    if (numShapes == 0) {
      // qDebug() << "Missing text data" << ticket;
      m_pivots.removeFirst();
      continue;
    }
    for (int i = 0; i < numShapes; ++i) {
      vs[10 * i + 8] = pivot.x();
      vs[10 * i + 9] = pivot.y();
    }
    instances.append(vs);
    m_pivots.removeFirst();
    m_instanceCount += numShapes;
  }
}


void S57::RasterHelper::setSymbolOffset(const QPointF &off) const {
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

void S57::VectorHelper::setSymbolOffset(const QPointF &off) const {
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
                                              const QPointF& offset,
                                              SymbolHelper* helper)
  : PaintData(t)
  , m_symbolType(s)
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
                                      const QPointF& offset,
                                      SymbolHelper* helper,
                                      const QPointF& pivot)
  : SymbolPaintDataBase(t, S52::SymbolType::Single, index, offset, helper)
{
  m_pivots << pivot.x() << pivot.y();
}

S57::RasterSymbolPaintData::RasterSymbolPaintData(quint32 index,
                                                  const QPointF& offset,
                                                  const QPointF& pivot,
                                                  const ElementData& elem)
  : SymbolPaintData(Type::RasterSymbols, index, offset, new RasterHelper(), pivot)
  , m_elem(elem)
{}

void S57::RasterSymbolPaintData::merge(const SymbolPaintDataBase* other, qreal, const KV::Region&) {
  if (other == nullptr) return;
  auto r = dynamic_cast<const RasterSymbolPaintData*>(other);
  Q_ASSERT(r->m_pivots.size() == 2);
  m_pivots.append(r->m_pivots);
  m_instanceCount += 1;
}

S57::VectorSymbolPaintData::VectorSymbolPaintData(quint32 index,
                                                  const QPointF& pivot,
                                                  const Angle& rot,
                                                  const KV::ColorVector& colors,
                                                  const ElementDataVector& elems)
  : SymbolPaintData(Type::VectorSymbols, index, QPoint(), new VectorHelper(), pivot)
{
  m_pivots << rot.cos() << rot.sin();
  for (int i = 0; i < colors.size(); i++) {
    ColorElementData d;
    d.color = colors[i];
    d.element = elems[i];
    m_elems.append(d);
    if (d.color.alpha() < 255) {
      m_type = Type::LucentVectorSymbols;
    }
  }
}

void S57::VectorSymbolPaintData::merge(const SymbolPaintDataBase* other, qreal, const KV::Region&) {
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
                                        const QPointF& offset,
                                        SymbolHelper* helper,
                                        const ElementDataVector& aelems,
                                        GLsizei aoffset,
                                        bool indexed,
                                        const QRectF& bbox,
                                        const PatternMMAdvance& advance)
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

void S57::PatternPaintData::merge(const SymbolPaintDataBase *other, qreal scale, const KV::Region& cover) {
  if (other == nullptr) {
    Q_ASSERT(m_areaElements.size() + m_areaArrays.size() == 1);
    for (const AreaData& a: m_areaElements) {
      auto reg = cover.intersected(a.bbox);
      if (reg.isEmpty()) continue;
      createPivots(reg.boundingRect(), scale);
    }
    for (const AreaData& a: m_areaArrays) {
      auto reg = cover.intersected(a.bbox);
      if (reg.isEmpty()) continue;
      createPivots(reg.boundingRect(), scale);
    }
  } else {
    auto r = dynamic_cast<const PatternPaintData*>(other);

    Q_ASSERT(r->m_areaElements.size() + r->m_areaArrays.size() == 1);
    for (const AreaData& a: r->m_areaElements) {
      auto reg = cover.intersected(a.bbox);
      if (reg.isEmpty()) continue;
      createPivots(reg.boundingRect(), scale);
    }
    for (const AreaData& a: r->m_areaArrays) {
      auto reg = cover.intersected(a.bbox);
      if (reg.isEmpty()) continue;
      createPivots(reg.boundingRect(), scale);
    }
    m_areaArrays.append(r->m_areaArrays);
    m_areaElements.append(r->m_areaElements);
  }
}


S57::RasterPatternPaintData::RasterPatternPaintData(quint32 index,
                                                    const QPointF& offset,
                                                    const ElementDataVector& aelems,
                                                    GLsizei aoffset,
                                                    bool indexed,
                                                    const QRectF& bbox,
                                                    const PatternMMAdvance& advance,
                                                    const ElementData& elem)
  : PatternPaintData(Type::RasterPatterns, index, offset, new RasterHelper(),
                     aelems, aoffset, indexed, bbox, advance)
  , m_elem(elem)
{}



void S57::RasterPatternPaintData::createPivots(const QRectF& bbox, qreal scale) {

  // transform millimeters (display) to meters (chart)
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
  m_instanceCount = m_pivots.size() / 2;
}


S57::VectorPatternPaintData::VectorPatternPaintData(quint32 index,
                                                    const ElementDataVector& aelems,
                                                    GLsizei aoffset,
                                                    bool indexed,
                                                    const QRectF& bbox,
                                                    const PatternMMAdvance& advance,
                                                    const Angle& rot,
                                                    const KV::ColorVector& colors,
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

  // transform millimeters (display) to meters (chart)
  // first scale nominal millimeters to actual millimeters
  scale *= Settings::instance()->displayLengthScaling();
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
  m_instanceCount = m_pivots.size() / 4;
}


S57::LineStylePaintData::LineStylePaintData(quint32 index,
                                            const ElementDataVector& lelems,
                                            GLsizei loffset,
                                            const QRectF& bbox,
                                            const PatternMMAdvance& advance,
                                            const KV::ColorVector& colors,
                                            const ElementDataVector& elems)
  : SymbolPaintDataBase(Type::VectorLineStyles, S52::SymbolType::LineStyle, index, QPoint(), new VectorHelper)
  , m_lineElements()
  , m_advance(advance.x)
  , m_cover()
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

void S57::LineStylePaintData::merge(const SymbolPaintDataBase* other, qreal scale, const KV::Region& cover) {
  if (other == nullptr) {
    // transform millimeters (display) to meters (chart)
    // first scale nominal millimeters to actual millimeters
    scale *= Settings::instance()->displayLengthScaling();
    m_advance /= scale;
    m_cover = cover;
    Q_ASSERT(m_lineElements.size() == 1);
  } else {
    auto r = dynamic_cast<const LineStylePaintData*>(other);
    Q_ASSERT(r && r->m_lineElements.size() == 1);
    m_lineElements.append(r->m_lineElements);
  }
}

void S57::LineStylePaintData::createTransforms(GL::VertexVector& transforms,
                                               GL::VertexVector& segments,
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

    auto reg = m_cover.intersected(d.bbox);
    if (reg.isEmpty()) continue;

    vs.offset = d.vertexOffset / sizeof(GLfloat) / 2;
    vs.count = (maxCoordOffset - d.vertexOffset) / sizeof(GLfloat) / 2;
    for (S57::ElementData elem: d.elements) {
      // account adjacency
      is.offset = elem.offset / sizeof(GLuint) + 1;
      is.count = elem.count - 2;
      lc->calculate(transforms, segments, m_advance, reg.boundingRect(), vs, is);
    }
  }

  m_instanceCount = (transforms.size() - m_pivotOffset / sizeof(GLfloat)) / 4;
}

void S57::LineStylePaintData::setColor(const QColor& c) const {
  m_helper->setColor(c);
}

LineKey S57::LineStylePaintData::key() const {
  QColor c("black");
  if (!m_elems.isEmpty()) {
    c = m_elems.first().color;
  }
  return LineKey(c, S52::LineType::Solid, S52::LineWidthMM(1));
}
