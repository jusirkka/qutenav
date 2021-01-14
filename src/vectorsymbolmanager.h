#pragma once

#include "types.h"
#include "s57object.h"
#include <QRect>
#include <QHash>
#include <QOpenGLBuffer>
#include "symboldata.h"


class QXmlStreamReader;


class VectorSymbolManager {

public:

  VectorSymbolManager();

  static VectorSymbolManager* instance();
  void createSymbols();

  SymbolData symbolData(quint32 index, S52::SymbolType type) const;

  void bind();

  ~VectorSymbolManager();


private:

  struct ParseData {
    QPoint offset;
    QPoint pivot;
    QSize size;
    int minDist;
    int maxDist;
  };

  using SymbolMap = QHash<SymbolKey, SymbolData>;

  void parseSymbols(QXmlStreamReader& reader,
                    GL::VertexVector& vertices,
                    GL::IndexVector& indices,
                    S52::SymbolType type);

  void parseSymbolData(QXmlStreamReader& reader,
                       ParseData& d,
                       QString& src);

  SymbolMap m_symbolMap;
  SymbolData m_invalid;
  QOpenGLBuffer m_coordBuffer;
  QOpenGLBuffer m_indexBuffer;
  const QStringList m_blacklist;
};
