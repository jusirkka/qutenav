#pragma once

#include <QVariant>
#include <QVector2D>
#include <QOpenGLFunctions>
#include <QColor>
#include <QRectF>
#include "types.h"
#include <QDebug>
#include "geoprojection.h"

class QOpenGLBuffer;

namespace CM93 {
class ObjectBuilder;
}

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

  virtual ~Base() = default;

protected:

  Base(Type t, const QPointF& c, const WGS84Point& cll)
    : m_type(t)
    , m_center(c)
    , m_centerLL(cll) {}

  Base(Type t): m_type(t) {}

  Type m_type;
  QPointF m_center;
  WGS84Point m_centerLL;

};


class Meta: public Base {
public:
  Meta(): Base(Type::Meta, QPointF(), WGS84Point()) {}
};

using PointVector = QVector<double>;

class Point: public Base {
public:
  Point(const QPointF& p, const GeoProjection* proj)
    : Base(Type::Point, p, proj->toWGS84(p)) {
    m_points.append(p.x());
    m_points.append(p.y());
  }

  Point(const PointVector& ps, const GeoProjection* proj)
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

  const PointVector& points() const {return m_points;}

private:

  PointVector m_points;
};


class Line: public Base {
public:
  Line(const ElementDataVector& elems,
       const QPointF& c,
       GLsizei vo,
       const GeoProjection* proj)
    : Base(Type::Line, c, proj->toWGS84(c))
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

private:

  ElementDataVector m_triangleElements;
  bool m_indexed;

};

} // namespace Geometry

class PaintData {
public:

  enum class Type {
    Override, // For a CS procedure to override the display settings
    Priority, // For a CS procedure to change priority
    TriangleElements,
    TriangleArrays,
    LineElements,
    LineArrays,
    LineLocal,
    TextElements,
    RasterSymbols,
    RasterPatterns,
    VectorSymbols,
    VectorPatterns,
    VectorLineStyles,
  };

  virtual void setUniforms() const = 0;
  virtual void setVertexOffset() const = 0;
  Type type() const {return m_type;}

  virtual ~PaintData() = default;

protected:

  PaintData(Type t);

  Type m_type;

};


class OverrideData: public PaintData {
public:
  void setUniforms() const override {/* noop */}
  void setVertexOffset() const override {/* noop */}

  OverrideData(bool uw);

  bool override() const {return m_override;}

protected:

  bool m_override;

};

class PriorityData: public PaintData {
public:
  void setUniforms() const override {/* noop */}
  void setVertexOffset() const override {/* noop */}

  PriorityData(int prio);

  int priority() const {return m_priority;}

protected:

  int m_priority;

};


class TriangleData: public PaintData {
public:
  void setUniforms() const override;
  void setVertexOffset() const override;

  const ElementDataVector& elements() const {return m_elements;}

protected:

  TriangleData(Type t, const ElementDataVector& elems, GLsizei offset, const QColor& c);

  ElementDataVector m_elements;
  GLsizei m_vertexOffset;
  QColor m_color;

};

class TriangleArrayData: public TriangleData {
public:
  TriangleArrayData(const ElementDataVector& elem, GLsizei offset, const QColor& c);
};

class TriangleElemData: public TriangleData {
public:
  TriangleElemData(const ElementDataVector& elem, GLsizei offset, const QColor& c);
};


class LineData: public PaintData {
public:

  const ElementDataVector& elements() const {return m_elements;}
  void setVertexOffset() const override;

  virtual void setStorageOffsets(uintptr_t offset) const = 0;

protected:

  LineData(Type t,
           const ElementDataVector& elems,
           GLsizei offset,
           const QColor& c,
           GLfloat lw,
           uint patt);

  ElementDataVector m_elements;
  GLfloat m_lineWidth;
  const GLfloat m_conv;
  GLsizei m_vertexOffset;
  QColor m_color;
  GLuint m_pattern;
};



class LineElemData: public LineData {
public:
  LineElemData(const ElementDataVector& elem,
               GLsizei offset,
               const QColor& c,
               GLfloat lw,
               uint pattern);

  void setUniforms() const override;
  void setStorageOffsets(uintptr_t offset) const override;
};

class LineArrayData: public LineData {
public:
  LineArrayData(const ElementDataVector& elem,
                GLsizei offset,
                const QColor& c,
                GLfloat lw,
                uint pattern);

  void setUniforms() const override;
  void setStorageOffsets(uintptr_t offset) const override;
};

class Globalizer {
public:
  virtual PaintData* globalize(GLsizei offset) const = 0;
  virtual GL::VertexVector vertices(qreal scale) = 0;
  virtual ~Globalizer() = default;
};


class LineLocalData: public LineData, public Globalizer {
public:
  LineLocalData(const GL::VertexVector& vertices,
                const ElementDataVector& elem,
                const QColor& c,
                GLfloat width,
                uint pattern,
                bool displayUnits,
                const QPointF& pivot);

  PaintData* globalize(GLsizei offset) const override;
  GL::VertexVector vertices(qreal scale) override;

  void setUniforms() const override;
  void setStorageOffsets(uintptr_t offset) const override;

private:
  GL::VertexVector m_vertices;
  bool m_displayUnits;
  QPointF m_pivot;
};


class TextElemData: public PaintData {
public:
  void setUniforms() const override;
  void setVertexOffset() const override;

  TextElemData(const QPointF& pivot,
               const QPointF& bboxBase,
               const QPointF& pivotOffset,
               const QPointF& bboxOffset,
               float boxScale,
               GLsizei vertexOffset,
               const ElementData& elems,
               const QColor& c);

  const ElementData& elements() const {return m_elements;}

protected:

  ElementData m_elements;
  GLsizei m_vertexOffset;
  QColor m_color;
  GLfloat m_scaleMM;
  QPointF m_pivot;
  QPointF m_shiftMM;

};

class SymbolHelper {
public:
  virtual void setSymbolOffset(const QPoint& off) const = 0;
  virtual void setVertexBufferOffset(GLsizei off) const = 0;
  virtual void setColor(const QColor& color) const = 0;
  virtual ~SymbolHelper() = default;
};


class RasterHelper: public SymbolHelper {
public:
  RasterHelper() = default;
  void setSymbolOffset(const QPoint& off) const override;
  void setVertexBufferOffset(GLsizei off) const override;
  void setColor(const QColor& color) const override;
};

class VectorHelper: public SymbolHelper {
public:
  VectorHelper() = default;
  void setSymbolOffset(const QPoint& off) const override;
  void setVertexBufferOffset(GLsizei off) const override;
  void setColor(const QColor& color) const override;
};

class SymbolPaintDataBase: public PaintData {

public:

  void setUniforms() const override;
  void setVertexOffset() const override;

  virtual void merge(const SymbolPaintDataBase* other, qreal scale, const QRectF& va) = 0;
  SymbolKey key() const {return SymbolKey(m_index, m_type);}
  void getPivots(GL::VertexVector& pivots);
  GLsizei count() const {return m_instanceCount;}

  virtual ~SymbolPaintDataBase();

protected:

  SymbolPaintDataBase(Type t,
                      S52::SymbolType s,
                      quint32 index,
                      const QPoint& offset,
                      SymbolHelper* helper);

  S52::SymbolType m_type;
  quint32 m_index;

  QPoint m_offset;

  SymbolHelper* m_helper;

  GLsizei m_pivotOffset;
  GL::VertexVector m_pivots;

  GLsizei m_instanceCount;

};



class SymbolPaintData: public SymbolPaintDataBase {

protected:

  SymbolPaintData(Type t,
                  quint32 index,
                  const QPoint& offset,
                  SymbolHelper* helper,
                  const QPointF& pivot);
};

class RasterSymbolPaintData: public SymbolPaintData {
public:
  void merge(const SymbolPaintDataBase* other, qreal, const QRectF&) override;
  RasterSymbolPaintData(quint32 index,
                        const QPoint& offset,
                        const QPointF& pivot,
                        const ElementData& elem);

  const ElementData& element() const {return m_elem;}

private:

  const ElementData& m_elem;
};


struct ColorElementData {
  QColor color;
  ElementData element;
};

using ColorElementVector = QVector<ColorElementData>;

class VectorSymbolPaintData: public SymbolPaintData {
public:
  void merge(const SymbolPaintDataBase* other, qreal, const QRectF& va) override;
  VectorSymbolPaintData(quint32 index,
                        const QPointF& pivot,
                        const Angle& rot,
                        const QT::ColorVector& colors,
                        const ElementDataVector& elems);

  const ColorElementVector& elements() const {return m_elems;}
  void setColor(const QColor& c) const;

private:
  ColorElementVector m_elems;
};


class PatternPaintData: public SymbolPaintDataBase {
public:

  void setAreaVertexOffset(GLsizei off) const;
  void merge(const SymbolPaintDataBase* other, qreal scale, const QRectF& va) override;

  struct AreaData {
    ElementDataVector elements;
    GLsizei vertexOffset;
    QRectF bbox;
  };

  using AreaDataVector = QVector<AreaData>;

  const AreaDataVector& areaElements() const {return m_areaElements;}
  const AreaDataVector& areaArrays() const {return m_areaArrays;}

  ~PatternPaintData() = default;

protected:

  PatternPaintData(Type t,
                   quint32 index,
                   const QPoint& offset,
                   SymbolHelper* helper,
                   const ElementDataVector& aelems,
                   GLsizei aoffset,
                   bool indexed,
                   const QRectF& bbox,
                   const PatternAdvance& advance);

  virtual void createPivots(const QRectF& bbox, qreal scale) = 0;

  AreaDataVector m_areaElements;
  AreaDataVector m_areaArrays;

  PatternAdvance m_advance;

};

class RasterPatternPaintData: public PatternPaintData {
public:
  RasterPatternPaintData(quint32 index,
                         const QPoint& offset,
                         const ElementDataVector& aelems,
                         GLsizei aoffset,
                         bool indexed,
                         const QRectF& bbox,
                         const PatternAdvance& advance,
                         const ElementData& elem);

  const ElementData& element() const {return m_elem;}

protected:

  void createPivots(const QRectF& bbox, qreal scale) override;

private:

  const ElementData& m_elem;

};

class VectorPatternPaintData: public PatternPaintData {
public:
  VectorPatternPaintData(quint32 index,
                         const ElementDataVector& aelems,
                         GLsizei aoffset,
                         bool indexed,
                         const QRectF& bbox,
                         const PatternAdvance& advance,
                         const Angle& rot,
                         const QT::ColorVector& colors,
                         const ElementDataVector& elems);

  const ColorElementVector& elements() const {return m_elems;}
  void setColor(const QColor& c) const;

protected:

  void createPivots(const QRectF& bbox, qreal scale) override;

private:

  ColorElementVector m_elems;
  qreal m_c;
  qreal m_s;

};


class LineStylePaintData: public SymbolPaintDataBase {
public:
  LineStylePaintData(quint32 index,
                     const ElementDataVector& lelems,
                     GLsizei loffset,
                     const QRectF& bbox,
                     const PatternAdvance& advance,
                     const QT::ColorVector& colors,
                     const ElementDataVector& elems);

  void merge(const SymbolPaintDataBase* other, qreal scale, const QRectF& va) override;

  void createTransforms(GL::VertexVector& transforms,
                        const QOpenGLBuffer& coordBuffer,
                        const QOpenGLBuffer& indexBuffer,
                        GLsizei maxCoordOffset);

  const ColorElementVector& elements() const {return m_elems;}
  void setColor(const QColor& c) const;

  struct LineData {
    ElementDataVector elements;
    GLsizei vertexOffset;
    QRectF bbox;
  };

  using LineDataVector = QVector<LineData>;
  const LineDataVector& lineElements() const {return m_lineElements;}

  ~LineStylePaintData() = default;

private:

  ColorElementVector m_elems;
  LineDataVector m_lineElements;
  qreal m_advance;
  QRectF m_viewArea;

};



using PaintDataMap = QMultiMap<PaintData::Type, PaintData*>;
using PaintIterator = QMultiMap<PaintData::Type, PaintData*>::const_iterator;
using PaintMutIterator = QMultiMap<PaintData::Type, PaintData*>::iterator;

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

  bool canPaint(const QRectF& viewArea, quint32 scale,
                const QDate& today, bool onlyViewArea) const;
  // for overridden checks
  bool canPaint(quint32 scale) const;

private:

  // shortcuts to find SCAMIN and date attribute values. From s57attributes.csv.
  static const int scaminIndex = 133;
  static const int datstaIndex = 86;
  static const int datendIndex = 85;
  static const int perstaIndex = 119;
  static const int perendIndex = 118;

  using IndexVector = QVector<quint32>;

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

