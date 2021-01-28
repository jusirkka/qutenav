#include "vectorsymbolmanager.h"
#include "s52presentation.h"
#include "s52names.h"
#include <QFile>
#include <QXmlStreamReader>
#include <QDebug>
#include "hpglparser.h"

VectorSymbolManager::VectorSymbolManager()
  : m_invalid()
  , m_coordBuffer(QOpenGLBuffer::VertexBuffer)
  , m_indexBuffer(QOpenGLBuffer::IndexBuffer)
  , m_blacklist {"BOYSPR01"}
{}


VectorSymbolManager* VectorSymbolManager::instance() {
  static VectorSymbolManager* m = new VectorSymbolManager();
  return m;
}

VectorSymbolManager::~VectorSymbolManager() {}

SymbolData VectorSymbolManager::symbolData(quint32 index, S52::SymbolType type) const {
  const SymbolKey key(index, type);
  if (m_symbolMap.contains(key)) return m_symbolMap[key];

  return m_invalid;
}

void VectorSymbolManager::bind() {
  m_indexBuffer.bind();
  m_coordBuffer.bind();
}


void VectorSymbolManager::createSymbols() {

  QFile file(S52::FindPath("chartsymbols.xml"));
  file.open(QFile::ReadOnly);
  QXmlStreamReader reader(&file);

  reader.readNextStartElement();
  Q_ASSERT(reader.name() == "chartsymbols");

  GL::VertexVector vertices;
  GL::IndexVector indices;
  while (reader.readNextStartElement()) {
    if (reader.name() == "line-styles") {
      parseSymbols(reader, vertices, indices, S52::SymbolType::LineStyle);
    } else if (reader.name() == "patterns") {
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


void VectorSymbolManager::parseSymbols(QXmlStreamReader& reader,
                                       GL::VertexVector& vertices,
                                       GL::IndexVector& indices,
                                       S52::SymbolType t) {
  const QString itemName = t == S52::SymbolType::Single ?
        "symbol" : t == S52::SymbolType::LineStyle ? "line-style" : "pattern";

  while (reader.readNextStartElement()) {
    Q_ASSERT(reader.name() == itemName);

    QString symbolName;
    QString cmap;
    QString src;
    ParseData d;
    bool staggered = false;

    while (reader.readNextStartElement()) {
      if (reader.name() == "name") {
        symbolName = reader.readElementText();
      } else if (reader.name() == "vector") {
        parseSymbolData(reader, d, src);
      } else if (reader.name() == "HPGL") {
        src = reader.readElementText();
      } else if (reader.name() == "color-ref") {
        cmap = reader.readElementText();
      } else if (reader.name() == "filltype") {
        staggered = reader.readElementText() != "L";
      } else {
        reader.skipCurrentElement();
      }
    }

    if (!d.size.isValid()) continue;

    if (m_blacklist.contains(symbolName)) {
      qWarning() << symbolName << "is blacklisted, skipping";
      continue;
    }

    HPGLParser parser(src, cmap, d.pivot);
    if (!parser.ok()) {
      qWarning() << "HPGLParser failed, skipping" << symbolName;
      continue;
    }

    S57::ElementDataVector elems;
    S52::ColorVector colors;

    for (const HPGLParser::Data& item: parser.data()) {
      colors.append(item.color);
      // create ElementData and append to elems
      S57::ElementData e;
      e.mode = GL_TRIANGLES;
      e.count = item.indices.size();
      e.offset = indices.size() * sizeof(GLuint);
      elems.append(e);
      // update vertices & indices
      const GLuint offset = vertices.size() / 2;
      vertices.append(item.vertices);
      for (GLuint i: item.indices) {
        indices << offset + i;
      }
    }

    if (d.maxDist < d.minDist) {
      qWarning() << "maxdist larger than mindist in" << symbolName;
    }
    SymbolData s(d.offset, d.size, d.minDist, staggered, elems, colors);

    const SymbolKey key(S52::FindIndex(symbolName), t);
    if (m_symbolMap.contains(key) && s != m_symbolMap[key]) {
      qWarning() << "multiple vector symbol/line-style/pattern definitions for"
                 << symbolName << ", skipping earlier";
    }
    m_symbolMap.insert(key, s);
  }
}

void VectorSymbolManager::parseSymbolData(QXmlStreamReader &reader,
                                          ParseData &d,
                                          QString& src) {
  Q_ASSERT(reader.name() == "vector");

  int w = reader.attributes().value("width").toInt();
  int h = reader.attributes().value("height").toInt();
  d.size = QSize(w, h);

  QPoint o;

  while (reader.readNextStartElement()) {
    if (reader.name() == "distance") {
      d.minDist = reader.attributes().value("min").toInt();
      d.maxDist = reader.attributes().value("max").toInt();
      reader.skipCurrentElement();
    } else if (reader.name() == "pivot") {
      d.pivot = QPoint(reader.attributes().value("x").toInt(),
                       reader.attributes().value("y").toInt());
      reader.skipCurrentElement();
    } else if (reader.name() == "origin") {
      o = QPoint(reader.attributes().value("x").toInt(),
                 reader.attributes().value("y").toInt());
      reader.skipCurrentElement();
    } else if (reader.name() == "HPGL") {
      src = reader.readElementText();
    } else {
      reader.skipCurrentElement();
    }
  }

  // offset of the upper left corner
  d.offset = QPoint(o.x() - d.pivot.x(), d.pivot.y() - o.y());

}
