/* -*- coding: utf-8-unix -*-
 *
 * File: src/s57object.h
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
#pragma once

#include <QVariant>
#include <QRectF>
#include "types.h"
#include "geoprojection.h"
#include <functional>

namespace KV {class Region;}

namespace S57 {

using Transform = std::function<QPointF (const QPointF&)>;


class Attribute {
public:

  using Type = AttributeType;

  Attribute(int v)
    : m_type(Type::Integer)
    , m_value(QVariant::fromValue(v)) {}

  Attribute(double v)
    : m_type(Type::Real)
    , m_value(QVariant::fromValue(v)) {}

  Attribute(const QVariantList& v)
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

  void encode(QDataStream& stream) const;
  static Attribute Decode(QDataStream& stream);

  bool matches(const Attribute& constraint) const;

private:
  Type m_type;
  QVariant m_value;
};

using AttributeMap = QMap<quint32, Attribute>;
using AttributeIterator = QMap<quint32, Attribute>::const_iterator;

struct ElementData {
  GLenum mode;
  // offset to vertex/index buffer, depending whether vertices are indexed or not
  // chart lines: always index buffer offset
  // generated lines: always vertex offset, i.e. 1st vertex to draw
  // unindexed triangles: vertex offset
  // indexed triangles: index buffer offset
  uintptr_t offset;
  size_t count;
  QRectF bbox;
};

using ElementDataVector = QVector<ElementData>;


namespace Geometry {

enum class Type: char {
  Point = 'P',
  Line = 'L',
  Area = 'A',
  Meta = 'M', // not really a geometry type
};

class Base {
public:

  Type type() const {return m_type;}
  const QPointF& center() const {return m_center;}
  const WGS84Point& centerLL() const {return m_centerLL;}

  void encode(QDataStream& stream, Transform transform) const;
  static Base* Decode(QDataStream& stream);

  virtual ~Base() = default;

protected:

  Base(Type t, const QPointF& c, const WGS84Point& cll)
    : m_type(t)
    , m_center(c)
    , m_centerLL(cll) {}

  Base(Type t): m_type(t) {}

  virtual void doEncode(QDataStream& stream, Transform transform) const = 0;
  virtual void doDecode(QDataStream& stream) = 0;

  Type m_type;
  QPointF m_center;
  WGS84Point m_centerLL;

};


class Meta: public Base {
public:
  Meta(): Base(Type::Meta, QPointF(), WGS84Point()) {}

protected:

  void doEncode(QDataStream &stream, Transform transform) const override;
  void doDecode(QDataStream& stream) override;
};


class Point: public Base {
public:

  Point(): Base(Type::Point) {}

  Point(const QPointF& p, const GeoProjection* proj)
    : Base(Type::Point, p, proj->toWGS84(p)) {
    m_points.append(p.x());
    m_points.append(p.y());
  }

  Point(const GL::VertexVector& ps, const GeoProjection* proj)
    : Base(Type::Point, QPointF(), WGS84Point())
    , m_points(ps) {
    QPointF s(0, 0);
    const int n = m_points.size() / 3;
    for (int i = 0; i < n; i++) {
      s.rx() += m_points[3 * i + 0];
      s.ry() += m_points[3 * i + 1];
    }
    m_center = s / n;
    m_centerLL = proj->toWGS84(m_center);
  }

  const GL::VertexVector& points() const {return m_points;}

  bool containedBy(const QRectF& box, int& index) const;

protected:

  void doEncode(QDataStream &stream, Transform transform) const override;
  void doDecode(QDataStream& stream) override;

private:

  GL::VertexVector m_points;
};


class Line: public Base {
public:

  Line(): Base(Type::Line) {}

  Line(const ElementDataVector& elems,
       const QPointF& c,
       GLsizei vo,
       const GeoProjection* proj)
    : Base(Type::Line, c, proj->toWGS84(c))
    , m_lineElements(elems)
    , m_vertexOffset(vo) {}

  const ElementDataVector& lineElements() const {return m_lineElements;}
  GLsizei vertexOffset() const {return m_vertexOffset;}

  bool crosses(const glm::vec2* vertices, const GLuint* indices, const QRectF& box) const;


protected:

  virtual void doEncode(QDataStream &stream, Transform transform) const override;
  virtual void doDecode(QDataStream& stream) override;

  ElementDataVector m_lineElements;
  GLsizei m_vertexOffset;

};

class Area: public Line {
public:

  Area(): Line() {m_type = Type::Area;}

  Area(const ElementDataVector& lelems,
       const QPointF& c,
       const ElementDataVector& telems,
       GLsizei vo,
       bool indexed,
       const GeoProjection* proj)
    : Line(lelems, c, vo, proj)
    , m_triangleElements(telems)
    , m_indexed(indexed)
  {
    m_type = Type::Area;
  }

  const ElementDataVector& triangleElements() const {return m_triangleElements;}
  bool indexed() const {return m_indexed;}

  bool includes(const glm::vec2* vs, const GLuint* is, const QPointF& p) const;


protected:

  void doEncode(QDataStream &stream, Transform transform) const override;
  void doDecode(QDataStream& stream) override;

private:

  ElementDataVector m_triangleElements;
  bool m_indexed;

};

} // namespace Geometry


class ObjectBuilder;

class Object;

using ObjectVector = QVector<Object*>;
using ObjectMap = QMap<quint32, Object*>;

class Object {

  friend class ObjectBuilder;

public:

  using LocationHash = QMultiHash<WGS84Point, const S57::Object*>;
  using LocationIterator = LocationHash::const_iterator;
  using ContourVector = QVector<double>;


  Object(quint32 fid, quint32 ftype)
    : m_feature_id(fid)
    , m_feature_type_code(ftype)
    , m_geometry(nullptr)
    , m_others(nullptr)
    , m_contours(nullptr)
  {}

  ~Object();

  QString name() const;

  quint32 classCode() const {return m_feature_type_code;}
  quint32 identifier() const {return m_feature_id;}
  const Geometry::Base* geometry() const {return m_geometry;}
  const AttributeMap& attributes() const {return m_attributes;}
  QVariant attributeValue(quint32 attr) const;
  QSet<int> attributeSetValue(quint32 attr) const;
  const QRectF& boundingBox() const {return m_bbox;}
  LocationIterator others() const {return m_others->find(m_geometry->centerLL());}
  LocationIterator othersEnd() const {return m_others->cend();}
  const ObjectVector& underlings() const {return m_underlings;}
  double getSafetyContour(double c0) const;

  bool canPaint(const KV::Region& cover, quint32 scale,
                const QDate& today, bool regionOnly) const;
  // for overridden checks
  bool canPaint(quint32 scale) const;

  void encode(QDataStream& stream, Transform transform) const;
  static Object* Decode(QDataStream& stream);

  S57::Description description(const WGS84Point& snd = WGS84Point(), qreal depth = 0.) const;

private:

  void decode(QDataStream& stream);

  // shortcuts to find SCAMIN and date attribute values. From s57attributes.csv.
  static const int scaminIndex = 133;
  static const int datstaIndex = 86;
  static const int datendIndex = 85;
  static const int perstaIndex = 119;
  static const int perendIndex = 118;

  const quint32 m_feature_id;
  const quint32 m_feature_type_code;
  AttributeMap m_attributes;
  Geometry::Base* m_geometry;
  QRectF m_bbox;
  LocationHash* m_others;
  ContourVector* m_contours;
  ObjectVector m_underlings;

};


}

