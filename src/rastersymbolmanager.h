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
    , advance(d.advance)
    , elements(d.elements) {}

  QPoint offset;
  QSize size;
  PatternAdvance advance;
  S57::ElementData elements;
};

class SymbolData {

  friend bool operator== (const SymbolData& s1, const SymbolData& s2);

public:


  SymbolData()
    : d(new SymbolSharedData) {}

  SymbolData(const QPoint& off, const QSize& size, int mnd, bool st, const S57::ElementData& elems);

  bool isValid() const {return d->size.isValid();}
  const QPoint& offset() const {return d->offset;}
  const QSize& size() const {return d->size;}
  const PatternAdvance& advance() const {return d->advance;}
  const S57::ElementData& elements() const {return d->elements;}

private:

  QSharedDataPointer<SymbolSharedData> d;

};

inline bool operator== (const SymbolData& s1, const SymbolData& s2) {
  if (s1.d->offset != s2.d->offset) return false;
  if (s1.d->size != s2.d->size) return false;
  if (s1.d->advance.x != s2.d->advance.x) return false;
  if (s1.d->advance.xy != s2.d->advance.xy) return false;
  if (s1.d->elements.offset != s2.d->elements.offset) return false;
  return true;
}

inline bool operator!= (const SymbolData& s1, const SymbolData& s2) {
  return !(s1 == s2);
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
  void parseSymbolData(QXmlStreamReader& reader, SymbolSharedData& d, int& minDist, VertexVector& vertices, IndexVector& indices);

  SymbolMap m_symbolMap;
  SymbolData m_invalid;
  QOpenGLBuffer m_coordBuffer;
  QOpenGLBuffer m_indexBuffer;
  QOpenGLTexture* m_symbolTexture;
};
