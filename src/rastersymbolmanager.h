#pragma once

#include "types.h"
#include "s57object.h"
#include <QRect>
#include <QHash>
#include <QOpenGLBuffer>
#include <QSharedData>

class QOpenGLTexture;
class QXmlStreamReader;

struct SymbolSharedData: public QSharedData {
  SymbolSharedData()
    : size() {}

  SymbolSharedData(const SymbolSharedData& d)
    : QSharedData(d)
    , offset(d.offset)
    , size(d.size)
    , minDist(d.minDist)
    , maxDist(d.maxDist)
    , staggered(d.staggered)
    , elements(d.elements) {}

  QPoint offset;
  QSize size;
  int minDist;
  int maxDist;
  bool staggered;
  S57::ElementData elements;
};

class SymbolData {

  friend bool operator== (const SymbolData& s1, const SymbolData& s2);

public:


  SymbolData()
    : d(new SymbolSharedData) {}

  SymbolData(const QPoint& off, const QSize& size, int mnd, int mxd, bool st, const S57::ElementData& elems)
    : d(new SymbolSharedData) {
    d->offset = off;
    d->size = size;
    d->minDist = mnd;
    d->maxDist = mxd;
    d->staggered = st;
    d->elements = elems;
  }

  bool isValid() const {return d->size.isValid();}
  const QPoint& offset() const {return d->offset;}
  const QSize& size() const {return d->size;}
  const S57::ElementData& elements() const {return d->elements;}

private:

  QSharedDataPointer<SymbolSharedData> d;

};

inline bool operator== (const SymbolData& s1, const SymbolData& s2) {
  if (s1.d->offset != s2.d->offset) return false;
  if (s1.d->size != s2.d->size) return false;
  if (s1.d->minDist != s2.d->minDist) return false;
  if (s1.d->maxDist != s2.d->maxDist) return false;
  if (s1.d->staggered != s2.d->staggered) return false;
  if (s1.d->elements.offset != s2.d->elements.offset) return false;
  return true;
}

inline bool operator!= (const SymbolData& s1, const SymbolData& s2) {
  return !(s1 == s2);
}


struct SymbolKey {

  SymbolKey(quint32 idx, S52::SymbolType s)
    : index(idx)
    , type(s) {}

  SymbolKey() = default;

  quint32 index;
  S52::SymbolType type;
};


inline bool operator== (const SymbolKey& k1, const SymbolKey& k2) {
  if (k1.index != k2.index) return false;
  return k1.type == k2.type;
}

inline uint qHash(const SymbolKey& key, uint seed) {
  return qHash(qMakePair(key.index, as_numeric(key.type)), seed);
}

class RasterSymbolManager {

public:

  RasterSymbolManager();

  static RasterSymbolManager* instance();
  void createSymbols();

  SymbolData symbolData(quint32 index, S52::SymbolType type) const;

  void bind();

  ~RasterSymbolManager();


private:

  using SymbolMap = QHash<SymbolKey, SymbolData>;
  using VertexVector = QVector<GLfloat>;
  using IndexVector = QVector<GLuint>;

  void parseSymbols(QXmlStreamReader& reader, VertexVector& vertices, IndexVector& indices);
  void parsePatterns(QXmlStreamReader& reader, VertexVector& vertices, IndexVector& indices);
  void parseSymbolData(QXmlStreamReader& reader, SymbolSharedData& d, VertexVector& vertices, IndexVector& indices);

  SymbolMap m_symbolMap;
  SymbolData m_invalid;
  QOpenGLBuffer m_coordBuffer;
  QOpenGLBuffer m_indexBuffer;
  QOpenGLTexture* m_symbolTexture;
};
