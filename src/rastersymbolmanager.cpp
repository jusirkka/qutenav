#include "rastersymbolmanager.h"
#include <QOpenGLTexture>
#include "s52presentation.h"
#include "s52names.h"
#include <QFile>
#include <QXmlStreamReader>
#include <QDebug>


RasterSymbolManager::RasterSymbolManager()
  : m_invalid()
  , m_coordBuffer(QOpenGLBuffer::VertexBuffer)
  , m_indexBuffer(QOpenGLBuffer::IndexBuffer)
  , m_symbolTexture(nullptr)
{}


RasterSymbolManager* RasterSymbolManager::instance() {
  static RasterSymbolManager* m = new RasterSymbolManager();
  return m;
}

RasterSymbolManager::~RasterSymbolManager() {
  delete m_symbolTexture;
}

SymbolData RasterSymbolManager::symbolData(quint32 index, S52::SymbolType type) const {
  const SymbolKey key(index, type);
  if (m_symbolMap.contains(key)) return m_symbolMap[key];

  return m_invalid;
}

void RasterSymbolManager::bind() {
  m_indexBuffer.bind();
  m_coordBuffer.bind();
  m_symbolTexture->bind();
}


void RasterSymbolManager::createSymbols() {

  m_symbolTexture = new QOpenGLTexture(QImage(S52::GetRasterFileName()), QOpenGLTexture::DontGenerateMipMaps);

  QFile file(S52::FindPath("chartsymbols.xml"));
  file.open(QFile::ReadOnly);
  QXmlStreamReader reader(&file);

  reader.readNextStartElement();
  Q_ASSERT(reader.name() == "chartsymbols");

  GL::VertexVector vertices;
  GL::IndexVector indices;
  // Note: there exists no raster line styles
  while (reader.readNextStartElement()) {
    if (reader.name() == "patterns") {
      parseSymbols(reader, vertices, indices, S52::SymbolType::Pattern);
    } else if (reader.name() == "symbols") {
      parseSymbols(reader, vertices, indices, S52::SymbolType::Single);
    } else {
      reader.skipCurrentElement();
    }
  }
  file.close();

  // fill in vertex buffer
  m_coordBuffer.create();
  m_coordBuffer.bind();
  m_coordBuffer.setUsagePattern(QOpenGLBuffer::StaticDraw);
  m_coordBuffer.allocate(vertices.constData(), sizeof(GLfloat) * vertices.size());

  // fill in index buffer
  m_indexBuffer.create();
  m_indexBuffer.bind();
  m_indexBuffer.setUsagePattern(QOpenGLBuffer::StaticDraw);
  m_indexBuffer.allocate(indices.constData(), sizeof(GLuint) * indices.size());
}

void RasterSymbolManager::parseSymbols(QXmlStreamReader &reader,
                                       GL::VertexVector &vertices,
                                       GL::IndexVector &indices,
                                       S52::SymbolType t) {

  const QString itemName = t == S52::SymbolType::Single ? "symbol" : "pattern";

  while (reader.readNextStartElement()) {
    Q_ASSERT(reader.name() == itemName);

    QString symbolName;
    ParseData d;
    bool staggered = false;
    bool skip = false;

    while (reader.readNextStartElement()) {
      if (reader.name() == "name") {
        symbolName = reader.readElementText();
      } else if (reader.name() == "filltype") {
        staggered = reader.readElementText() != "L";
      } else if (reader.name() == "bitmap") {
        parseSymbolData(reader, d, vertices, indices);
      } else if (reader.name() == "prefer-bitmap") {
        skip = reader.readElementText() == "no";
      } else {
        reader.skipCurrentElement();
      }
    }

    if (skip || !d.size.isValid()) continue;

    if (d.maxDist < d.minDist) {
      qWarning() << "maxdist larger than mindist in" << symbolName;
    }
    SymbolData s(d.offset, d.size, d.minDist, staggered, d.elements);

    const SymbolKey key(S52::FindIndex(symbolName), t);
    if (m_symbolMap.contains(key) && s != m_symbolMap[key]) {
      qWarning() << "multiple raster symbol/pattern definitions for"
                 << symbolName << ", skipping earlier";
    }
    m_symbolMap.insert(key, s);
  }
}


void RasterSymbolManager::parseSymbolData(QXmlStreamReader &reader,
                                          ParseData &d,
                                          GL::VertexVector &vertices,
                                          GL::IndexVector &indices) {
  Q_ASSERT(reader.name() == "bitmap");

  int w = reader.attributes().value("width").toInt();
  int h = reader.attributes().value("height").toInt();
  d.size = QSize(w, h);

  QPoint p;
  QPoint o;

  QPointF t0;
  const float W = m_symbolTexture->width();
  const float H = m_symbolTexture->height();

  while (reader.readNextStartElement()) {
    if (reader.name() == "distance") {
      d.minDist = reader.attributes().value("min").toInt();
      d.maxDist = reader.attributes().value("max").toInt();
      reader.skipCurrentElement();
    } else if (reader.name() == "pivot") {
      p = QPoint(reader.attributes().value("x").toInt(),
                 reader.attributes().value("y").toInt());
      reader.skipCurrentElement();
    } else if (reader.name() == "origin") {
      o = QPoint(reader.attributes().value("x").toInt(),
                 reader.attributes().value("y").toInt());
      reader.skipCurrentElement();
    } else if (reader.name() == "graphics-location") {
      t0 = QPointF(reader.attributes().value("x").toInt() / W,
                   reader.attributes().value("y").toInt() / H);
      reader.skipCurrentElement();
    } else {
      reader.skipCurrentElement();
    }
  }

  // offset of the upper left corner
  d.offset = QPoint(o.x() - p.x(), p.y() - o.y());
  d.elements = S57::ElementData {GL_TRIANGLES, indices.size() * sizeof(GLuint), 6};


  const GLuint ioff = vertices.size() / 4;
  indices << 0 + ioff << 1 + ioff << 2 + ioff << 0 + ioff << 2 + ioff << 3 + ioff;

  // upper left
  const QPointF p0(0, 0);
  // lower right
  const QPointF p1 = p0 + QPointF(w, - h);
  const QPointF t1 = t0 + QPointF(w / W, h / H);

  vertices << p0.x() << p0.y() << t0.x() << t0.y();
  vertices << p1.x() << p0.y() << t1.x() << t0.y();
  vertices << p1.x() << p1.y() << t1.x() << t1.y();
  vertices << p0.x() << p1.y() << t0.x() << t1.y();

}
