#pragma once

#include "types.h"
#include "s57object.h"
#include <QRect>
#include <QHash>
#include <QOpenGLBuffer>
#include "symboldata.h"


class QOpenGLTexture;
class QXmlStreamReader;


class RasterSymbolManager: public QObject {

  Q_OBJECT

public:

  RasterSymbolManager();

  static RasterSymbolManager* instance();
  void createSymbols();

  SymbolData symbolData(quint32 index, S52::SymbolType type) const;

  void bind();

  ~RasterSymbolManager();

private slots:

  void changeSymbolAtlas();

private:

  struct ParseData {
    QPoint offset;
    QSize size;
    int minDist;
    int maxDist;
    S57::ElementData elements;
  };

  using SymbolMap = QHash<SymbolKey, SymbolData>;

  void parseSymbols(QXmlStreamReader& reader,
                    GL::VertexVector& vertices,
                    GL::IndexVector& indices,
                    S52::SymbolType type);

  void parseSymbolData(QXmlStreamReader& reader,
                       ParseData& d,
                       GL::VertexVector& vertices,
                       GL::IndexVector& indices);

  SymbolMap m_symbolMap;
  SymbolData m_invalid;
  QOpenGLBuffer m_coordBuffer;
  QOpenGLBuffer m_indexBuffer;
  QOpenGLTexture* m_symbolTexture;
  QString m_symbolAtlas;
};
