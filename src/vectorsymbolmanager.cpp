/* -*- coding: utf-8-unix -*-
 *
 * File: src/vectorsymbolmanager.cpp
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
#include "vectorsymbolmanager.h"
#include "s52presentation.h"
#include "s52names.h"
#include <QFile>
#include <QXmlStreamReader>
#include "logging.h"
#include "hpglopenglparser.h"
#include "hpglpixmapparser.h"
#include <QPainter>

VectorSymbolManager::VectorSymbolManager()
  : m_invalid()
  , m_coordBuffer(QOpenGLBuffer::VertexBuffer)
  , m_indexBuffer(QOpenGLBuffer::IndexBuffer)
  , m_pixmapCache(100 * sizeof(QPixmap))
{}

void VectorSymbolManager::init() {
  qCDebug(CDPY) << "init (noop)";
}

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

  if (m_coordBuffer.isCreated()) return;

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

    HPGL::OpenGLParser parser(src, cmap, d.pivot);
    if (!parser.ok()) {
      qCWarning(CSYM) << "HPGLParser failed, skipping" << symbolName;
      continue;
    }

    S57::ElementDataVector elems;
    S52::ColorVector colors;

    for (const HPGL::OpenGLParser::Data& item: parser.data()) {
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
      qCWarning(CSYM) << "maxdist larger than mindist in" << symbolName;
    }
    SymbolData s(d.offset, d.size, d.minDist, staggered, elems, colors);

    const SymbolKey key(S52::FindIndex(symbolName), t);
    if (m_symbolMap.contains(key) && s != m_symbolMap[key]) {
      qCWarning(CSYM) << "multiple vector symbol/line-style/pattern definitions for"
                      << symbolName << ", skipping earlier";
    }
    m_symbolMap.insert(key, s);
    m_painterData.insert(key, PainterData(src, cmap, d.center));
  }
}

void VectorSymbolManager::parseSymbolData(QXmlStreamReader &reader,
                                          ParseData &d,
                                          QString& src) {
  Q_ASSERT(reader.name() == "vector");

  int w = reader.attributes().value("width").toInt();
  int h = reader.attributes().value("height").toInt();
  d.size = QSizeF(mmUnit * w, mmUnit * h);

  QPointF o;

  while (reader.readNextStartElement()) {
    if (reader.name() == "distance") {
      d.minDist = reader.attributes().value("min").toInt() * mmUnit;
      d.maxDist = reader.attributes().value("max").toInt() * mmUnit;
      reader.skipCurrentElement();
    } else if (reader.name() == "pivot") {
      d.pivot = QPointF(reader.attributes().value("x").toInt() * mmUnit,
                        reader.attributes().value("y").toInt() * mmUnit);
      reader.skipCurrentElement();
    } else if (reader.name() == "origin") {
      d.center = QPoint(reader.attributes().value("x").toInt(),
                        reader.attributes().value("y").toInt());
      o = QPointF(d.center.x() * mmUnit, d.center.y() * mmUnit);
      reader.skipCurrentElement();
    } else if (reader.name() == "HPGL") {
      src = reader.readElementText();
    } else {
      reader.skipCurrentElement();
    }
  }

  // offset of the upper left corner
  d.offset = QPointF(o.x() - d.pivot.x(),
                     d.pivot.y() - o.y());

}

bool VectorSymbolManager::paintIcon(QPainter& painter,
                                    quint32 index, S52::SymbolType type,
                                    qint16 angle) {
  const CacheKey key(index, type, angle);
  if (!m_pixmapCache.contains(key)) {
    const SymbolKey key0 = key.key();
    if (!m_painterData.contains(key0)) return false;
    auto pix = new QPixmap;
    const PainterData& d = m_painterData[key0];
    HPGL::PixmapParser parser(d.src, d.cmap, angle);
    *pix = parser.pixmap();
    m_pixmapCache.insert(key, pix);
  }
  QPixmap pix = *m_pixmapCache[key];
  if (pix.width() > .75 * painter.device()->width()) {
    pix = pix.scaledToWidth(.75 * painter.device()->width());
  }
  if (pix.height() > .75 * painter.device()->height()) {
    pix = pix.scaledToHeight(.75 * painter.device()->height());
  }
  const auto ox = (painter.device()->width() - pix.width()) / 2;
  const auto oy = (painter.device()->height() - pix.height()) / 2;
  painter.drawPixmap(ox, oy, pix);
  return true;
}
