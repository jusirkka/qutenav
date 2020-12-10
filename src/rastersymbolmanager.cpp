#include "rastersymbolmanager.h"
#include <QOpenGLTexture>
#include "s52presentation.h"
#include <QFile>
#include <QXmlStreamReader>
#include <QDebug>

SymbolData::SymbolData(const QPoint& off, const QSize& size, int mnd, bool st, const S57::ElementData& elems)
  : d(new SymbolSharedData) {

  d->offset = off;
  d->size = size;
  d->elements = elems;

  // check if pivot is outside of the rect(offset, w, h) and enlarge
  // note: inverted y-axis
  const QPointF origin(off.x(), off.y() - size.height());
  const QRectF rtest(origin, size);
  auto r1 = rtest.united(QRectF(QPointF(0, 0), QSizeF(.1, .1)));
  const int x0 = r1.width() + mnd;
  const int y0 = r1.height() + mnd;
  const int x1 = st ? .5 * x0 : 0.;
  d->advance = PatternAdvance(x0, y0, x1);
}


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

  VertexVector vertices;
  IndexVector indices;
  // Note: there exists no raster line styles
  while (reader.readNextStartElement()) {
    if (reader.name() == "patterns") {
      parsePatterns(reader, vertices, indices);
    } else if (reader.name() == "symbols") {
      parseSymbols(reader, vertices, indices);
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
  m_indexBuffer.allocate(indices.constData() ,sizeof(GLuint) * indices.size());
}

void RasterSymbolManager::parsePatterns(QXmlStreamReader &reader, VertexVector &vertices, IndexVector &indices) {
  while (reader.readNextStartElement()) {
    Q_ASSERT(reader.name() == "pattern");

    QString patternName;
    SymbolSharedData d;
    bool staggered;
    int minDist;

    while (reader.readNextStartElement()) {
      if (reader.name() == "name") {
        patternName = reader.readElementText();
      } else if (reader.name() == "filltype") {
        staggered = reader.readElementText() != "L";
      } else if (reader.name() == "bitmap") {
        parseSymbolData(reader, d, minDist, vertices, indices);
        break;
      } else {
        reader.skipCurrentElement();
      }
    }
    if (d.size.isValid()) {
      // bitmap data found: skip rest of the element
      reader.skipCurrentElement();
    } else {
      continue;
    }

    SymbolData s(d.offset, d.size, minDist, staggered, d.elements);

    const SymbolKey key(S52::FindIndex(patternName), S52::SymbolType::Pattern);
    if (m_symbolMap.contains(key)) {
      if (s != m_symbolMap[key]) {
        qWarning() << "multiple raster pattern definitions for" << patternName << ", skipping latest";
      }
      continue;
    }
    m_symbolMap.insert(key, s);

  }
}

void RasterSymbolManager::parseSymbols(QXmlStreamReader &reader, VertexVector &vertices, IndexVector &indices) {
  while (reader.readNextStartElement()) {
    Q_ASSERT(reader.name() == "symbol");

    QString symbolName;
    SymbolSharedData d;
    int minDist;

    while (reader.readNextStartElement()) {
      if (reader.name() == "name") {
        symbolName = reader.readElementText();
      } else if (reader.name() == "bitmap") {
        parseSymbolData(reader, d, minDist, vertices, indices);
        break;
      } else {
        reader.skipCurrentElement();
      }
    }
    if (d.size.isValid()) {
      // bitmap data found: skip rest of the element
      reader.skipCurrentElement();
    } else {
      continue;
    }

    SymbolData s(d.offset, d.size, 0, false, d.elements);

    const SymbolKey key(S52::FindIndex(symbolName), S52::SymbolType::Single);
    if (m_symbolMap.contains(key)) {
      if (s != m_symbolMap[key]) {
        qWarning() << "multiple raster symbol definitions for" << symbolName << ", skipping latest";
      }
      continue;
    }
    m_symbolMap.insert(key, s);
  }
}


void RasterSymbolManager::parseSymbolData(QXmlStreamReader &reader, SymbolSharedData &d, int& minDist, VertexVector &vertices, IndexVector &indices) {
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
      minDist = reader.attributes().value("min").toInt();
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
