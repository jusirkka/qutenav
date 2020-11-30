#include "s57object.h"
#include <QDate>
#include <QDebug>
#include "shader.h"



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

  // Compare integer lists up to constraint list size
  if (m_type == Type::IntegerList) {
    QList<QVariant> clist = constraint.value().toList();
    QList<QVariant> mlist = m_value.toList();
    if (mlist.size() < clist.size()) return false;
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

QString S57::Object::name() const {
  auto s = QString("%1-%2-%3").arg(identifier()).arg(classCode()).arg(char(geometry()->type()));
  return s;
}

QVariant S57::Object::attributeValue(quint32 attr) const {
  if (!m_attributes.contains(attr)) return QVariant();
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
      // qDebug() << "scale too big" << scale << mx;
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
  prog->prog()->setUniformValue(prog->m_locations.lineWidth, m_lineWidth);
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
  prog->prog()->setUniformValue(prog->m_locations.lineWidth, m_lineWidth);
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

S57::SolidLineArrayData* S57::SolidLineLocalData::createArrayData(GLsizei offset) const {
  return new SolidLineArrayData(m_elements, offset, m_color, m_lineWidth);
}


S57::DashedLineLocalData::DashedLineLocalData(const VertexVector& vertices, const ElementDataVector& elem, const QColor& c, GLfloat width, uint pattern)
  : DashedLineData(Type::DashedLineLocal, elem, 0, c, width, pattern)
  , m_vertices(vertices)
{}

S57::DashedLineArrayData* S57::DashedLineLocalData::createArrayData(GLsizei offset) const {
  return new DashedLineArrayData(m_elements, offset, m_color, m_lineWidth, m_pattern);
}

