#pragma once

#include <QVariant>
#include <QVector2D>
#include <QOpenGLFunctions>
#include <QColor>

namespace S57 {

class Attribute {
public:

  enum class Type: uint8_t {
    Integer,
    IntegerList,
    Real,
    None, // OgrAttr_t has RealLst here
    String,
    Any // Not in OgrAttr_t
  };

  Attribute(int v)
    : m_type(Type::Integer)
    , m_value(QVariant::fromValue(v)) {}

  Attribute(double v)
    : m_type(Type::Real)
    , m_value(QVariant::fromValue(v)) {}

  Attribute(const QVector<int>& v)
    : m_type(Type::IntegerList)
    , m_value(QVariant::fromValue(v)) {}

  Attribute(const QString& v)
    : m_type(Type::String)
    , m_value(QVariant::fromValue(v)) {}

  Attribute(Type t)
    : m_type(t)
    , m_value() {}

  Attribute() = default;

  Type type() const {return m_type;}
  const QVariant& value() const {return m_value;}

  bool matches(const Attribute& constraint) const;

private:
  Type m_type;
  QVariant m_value;
};

using AttributeMap = QMap<quint32, Attribute>;
using AttributeIterator = QMap<quint32, Attribute>::const_iterator;

struct ElementData {
  // lines, lineloop, linestrip, triangles, trianglefan, trianglestrip
  GLenum mode;
  uintptr_t elementOffset;
  size_t elementCount;
};

using ElementDataVector = QVector<ElementData>;

namespace Geometry {

enum class Type: char {
  Point = 'P',
  Line = 'L',
  Area = 'A',
  Meta, // not really a geometry type
};

class Base {
public:

  Type type() const {return m_type;}

  virtual ~Base() = default;

protected:

  Base(Type t): m_type(t) {}

  Type m_type;

};


class Meta: public Base {
public:
  Meta(): Base(Type::Meta) {}
};

using PointVector = QVector<double>;

class Point: public Base {
public:
  Point(const QVector2D& p)
    : Base(Type::Point) {
    m_points.append(p.x());
    m_points.append(p.y());
    m_points.append(0.);
  }

  Point(const PointVector& ps)
    : Base(Type::Point)
    , m_points(ps) {}

  const PointVector& points() const {return m_points;}

private:

  PointVector m_points;
};


class Line: public Base {
public:
  Line(const ElementDataVector& elems, GLsizei vo)
    : Base(Type::Line)
    , m_lineElements(elems)
    , m_vertexOffset(vo) {}

  const ElementDataVector& lineElements() const {return m_lineElements;}
  GLsizei vertexOffset() const {return m_vertexOffset;}

private:

  ElementDataVector m_lineElements;
  GLsizei m_vertexOffset;

};

class Area: public Line {
public:
  Area(const ElementDataVector& lelems, const ElementDataVector& telems, GLsizei vo)
    : Line(lelems, vo)
    , m_triangleElements(telems)
  {
    m_type = Type::Area;
  }

  const ElementDataVector& triangleElements() const {return m_triangleElements;}

private:

  ElementDataVector m_triangleElements;

};

} // namespace Geometry


struct PaintData {
  enum class Type {
    Invalid,
    Lines,
    Triangles,
  };

  Type type;
  QColor color;
  GLsizei vertexOffset;
  ElementDataVector elements;
};

using PaintDataVector = QVector<PaintData>;
using PaintDataMap = QMap<PaintData::Type, PaintData>;
using PaintDataVectorMap = QMap<PaintData::Type, PaintDataVector>;

class ObjectBuilder;

class Object {

  friend class ObjectBuilder;

public:


  Object(quint32 fid, quint32 ftype)
    : m_feature_id(fid)
    , m_feature_type_code(ftype)
    , m_geometry(nullptr) {}

  quint32 classCode() const {return m_feature_type_code;}
  quint32 identifier() const {return m_feature_id;}
  const Geometry::Base* geometry() const {return m_geometry;}
  const AttributeMap& attributes() const {return m_attributes;}

private:

  const quint32 m_feature_id;
  const quint32 m_feature_type_code;
  AttributeMap m_attributes;
  Geometry::Base* m_geometry;
};


}

