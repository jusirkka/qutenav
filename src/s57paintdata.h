/* -*- coding: utf-8-unix -*-
 *
 * File: src/s57paintdata.h
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

#include <QMultiMap>
#include <QOpenGLBuffer>
#include <QColor>
#include "s57object.h"
#include "region.h"

namespace S57 {

class PaintData {
public:

  enum class Type {
    Override, // For a CS procedure to override the display settings
    Priority, // For a CS procedure to change priority
    TriangleElements,
    TriangleArrays,
    LucentTriangleElements,
    LucentTriangleArrays,
    LineElements, // Indexed linestrings
    LucentLineElements,
    LineArrays, // Plain linestrings
    LucentLineArrays,
    SegmentArrays, // disconnected segments
    LineLocal, // Plain linestrings, paintdata includes the vertices
    TextElements,
    RasterSymbols,
    RasterPatterns,
    VectorSymbols,
    LucentVectorSymbols,
    VectorPatterns,
    VectorLineStyles,
  };

  virtual void setUniforms() const = 0;
  virtual void setVertexOffset() const = 0;
  virtual void filterElements(const KV::Region&) {/* noop */}
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
  void filterElements(const KV::Region& viewArea) override;

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

  void filterElements(const KV::Region& viewArea) override;


protected:

  LineData(Type t,
           const ElementDataVector& elems,
           GLsizei offset,
           const QColor& c,
           GLfloat lw,
           uint patt);

  ElementDataVector m_elements;
  GLfloat m_lineWidth;
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

class SegmentArrayData: public LineData {
public:
  SegmentArrayData(int count,
                   GLsizei offset,
                   const QColor& c,
                   GLfloat lw,
                   uint pattern);

  void setUniforms() const override;
  void setStorageOffsets(uintptr_t offset) const override;
};

class Globalizer {
public:
  virtual PaintData* globalize(GLsizei offset, qreal scale) const = 0;
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

  PaintData* globalize(GLsizei offset, qreal scale) const override;
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
               int ticket,
               const QColor& c);

  void merge(const TextElemData* other);
  void getInstances(GL::VertexVector& instances);

  QColor color() const {return m_color;}
  int count() const {return m_instanceCount;}


protected:

  using TicketVector = QVector<int>;
  using PointVector = QVector<QPointF>;

  PointVector m_pivots;
  TicketVector m_tickets;
  QColor m_color;
  GLsizei m_instanceOffset;
  int m_instanceCount;

};

class SymbolHelper {
public:
  virtual void setSymbolOffset(const QPointF& off) const = 0;
  virtual void setVertexBufferOffset(GLsizei off) const = 0;
  virtual void setColor(const QColor& color) const = 0;
  virtual ~SymbolHelper() = default;
};


class RasterHelper: public SymbolHelper {
public:
  RasterHelper() = default;
  void setSymbolOffset(const QPointF& off) const override;
  void setVertexBufferOffset(GLsizei off) const override;
  void setColor(const QColor& color) const override;
};

class VectorHelper: public SymbolHelper {
public:
  VectorHelper() = default;
  void setSymbolOffset(const QPointF& off) const override;
  void setVertexBufferOffset(GLsizei off) const override;
  void setColor(const QColor& color) const override;
};

class SymbolPaintDataBase: public PaintData {

public:

  void setUniforms() const override;
  void setVertexOffset() const override;

  virtual void merge(const SymbolPaintDataBase* other, qreal scale, const KV::Region& va) = 0;
  SymbolKey key() const {return SymbolKey(m_index, m_symbolType);}
  void getPivots(GL::VertexVector& pivots);
  GLsizei count() const {return m_instanceCount;}

  virtual ~SymbolPaintDataBase();

protected:

  SymbolPaintDataBase(Type t,
                      S52::SymbolType s,
                      quint32 index,
                      const QPointF& offset,
                      SymbolHelper* helper);

  S52::SymbolType m_symbolType;
  quint32 m_index;

  QPointF m_offset;

  SymbolHelper* m_helper;

  GLsizei m_pivotOffset;
  GL::VertexVector m_pivots;

  GLsizei m_instanceCount;

};



class SymbolPaintData: public SymbolPaintDataBase {

protected:

  SymbolPaintData(Type t,
                  quint32 index,
                  const QPointF& offset,
                  SymbolHelper* helper,
                  const QPointF& pivot);
};

class RasterSymbolPaintData: public SymbolPaintData {
public:
  void merge(const SymbolPaintDataBase* other, qreal, const KV::Region&) override;
  RasterSymbolPaintData(quint32 index,
                        const QPointF& offset,
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
  void merge(const SymbolPaintDataBase* other, qreal, const KV::Region& va) override;
  VectorSymbolPaintData(quint32 index,
                        const QPointF& pivot,
                        const Angle& rot,
                        const KV::ColorVector& colors,
                        const ElementDataVector& elems);

  const ColorElementVector& elements() const {return m_elems;}
  void setColor(const QColor& c) const;

private:
  ColorElementVector m_elems;
};


class PatternPaintData: public SymbolPaintDataBase {
public:

  void setAreaVertexOffset(GLsizei off) const;
  void merge(const SymbolPaintDataBase* other, qreal scale, const KV::Region& va) override;

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
                   const QPointF& offset,
                   SymbolHelper* helper,
                   const ElementDataVector& aelems,
                   GLsizei aoffset,
                   bool indexed,
                   const QRectF& bbox,
                   const PatternMMAdvance& advance);

  virtual void createPivots(const QRectF& bbox, qreal scale) = 0;

  AreaDataVector m_areaElements;
  AreaDataVector m_areaArrays;

  PatternMMAdvance m_advance;

};

class RasterPatternPaintData: public PatternPaintData {
public:
  RasterPatternPaintData(quint32 index,
                         const QPointF& offset,
                         const ElementDataVector& aelems,
                         GLsizei aoffset,
                         bool indexed,
                         const QRectF& bbox,
                         const PatternMMAdvance& advance,
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
                         const PatternMMAdvance& advance,
                         const Angle& rot,
                         const KV::ColorVector& colors,
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
                     const PatternMMAdvance& advance,
                     const KV::ColorVector& colors,
                     const ElementDataVector& elems);

  void merge(const SymbolPaintDataBase* other, qreal scale, const KV::Region& va) override;

  void createTransforms(GL::VertexVector& transforms,
                        GL::VertexVector& segments,
                        const QOpenGLBuffer& coordBuffer,
                        const QOpenGLBuffer& indexBuffer,
                        GLsizei maxCoordOffset);

  LineKey key() const;

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
  KV::Region m_cover;

};



using PaintDataMap = QMultiMap<PaintData::Type, PaintData*>;
using PaintIterator = PaintDataMap::const_iterator;
using PaintMutIterator = PaintDataMap::iterator;

}
