/* -*- coding: utf-8-unix -*-
 *
 * File: src/rastersymbolmanager.cpp
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
#include "rastersymbolmanager.h"
#include <QOpenGLTexture>
#include "s52presentation.h"
#include "s52names.h"
#include <QFile>
#include <QXmlStreamReader>
#include "logging.h"
#include "settings.h"
#include <QPainter>
#include <QOpenGLExtraFunctions>
#include "platform.h"

RasterSymbolManager::RasterSymbolManager()
  : QObject()
  , m_invalid()
  , m_coordBuffer(QOpenGLBuffer::VertexBuffer)
  , m_indexBuffer(QOpenGLBuffer::IndexBuffer)
  , m_symbolTexture(nullptr)
  , m_symbolAtlas()
  , m_pixmapCache(100 * sizeof(QPixmap))
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
  m_coordBuffer.bind();
  m_indexBuffer.bind();
  m_symbolTexture->bind();
}

void RasterSymbolManager::changeSymbolAtlas() {
  auto rname = S52::GetRasterFileName();
  if (rname == m_symbolAtlas) return;
  m_symbolAtlas = rname;
  if (m_symbolTexture != nullptr) {
    // bare delete without destroying underlying resources first leads to a segfault in
    // sfos/qt5.6. A bug?
    qCDebug(CDPY) << "destroy texture";
    m_symbolTexture->destroy();
    qCDebug(CDPY) << "delete texture";
    delete m_symbolTexture;
  }
  qCDebug(CDPY) << "new texture";
  m_symbolTexture = new QOpenGLTexture(QImage(m_symbolAtlas), QOpenGLTexture::DontGenerateMipMaps);
  m_symbolTexture->setWrapMode(QOpenGLTexture::ClampToEdge);
  m_symbolTexture->setMinMagFilters(QOpenGLTexture::Linear, QOpenGLTexture::Linear);
}

void RasterSymbolManager::createSymbols() {

  changeSymbolAtlas();

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
      qCWarning(CS52) << "maxdist larger than mindist in" << symbolName;
    }
    SymbolData s(d.offset, d.size, d.minDist, staggered, d.elements);

    const SymbolKey key(S52::FindIndex(symbolName), t);
    if (m_symbolMap.contains(key) && s != m_symbolMap[key]) {
      qCWarning(CS52) << "multiple raster symbol/pattern definitions for"
                      << symbolName << ", skipping earlier";
    }
    m_symbolMap.insert(key, s);
    m_painterData.insert(key, PainterData(d.graphicsLocation, d.graphicsOffset));
  }
}


void RasterSymbolManager::parseSymbolData(QXmlStreamReader &reader,
                                          ParseData &d,
                                          GL::VertexVector &vertices,
                                          GL::IndexVector &indices) {
  Q_ASSERT(reader.name() == "bitmap");

  int w = reader.attributes().value("width").toInt();
  int h = reader.attributes().value("height").toInt();
  d.size = QSizeF(w / dots_per_mm_x(), h  / dots_per_mm_y());

  QPoint p;
  QPoint o;

  QPointF t0;
  const qreal W = m_symbolTexture->width();
  const qreal H = m_symbolTexture->height();

  while (reader.readNextStartElement()) {
    if (reader.name() == "distance") {
      d.minDist = reader.attributes().value("min").toInt() / dots_per_mm_x();
      d.maxDist = reader.attributes().value("max").toInt() / dots_per_mm_x();
      reader.skipCurrentElement();
    } else if (reader.name() == "pivot") {
      p = QPoint(reader.attributes().value("x").toInt(),
                 reader.attributes().value("y").toInt());
      d.graphicsOffset = p;
      reader.skipCurrentElement();
    } else if (reader.name() == "origin") {
      o = QPoint(reader.attributes().value("x").toInt(),
                 reader.attributes().value("y").toInt());
      reader.skipCurrentElement();
    } else if (reader.name() == "graphics-location") {
      const auto x = reader.attributes().value("x").toInt();
      const auto y = reader.attributes().value("y").toInt();
      t0 = QPointF(x / W, y / H);
      d.graphicsLocation = QRect(x, y, w, h);
      reader.skipCurrentElement();
    } else {
      reader.skipCurrentElement();
    }
  }

  // offset of the upper left corner
  d.offset = QPointF((o.x() - p.x()) / dots_per_mm_x(),
                     (p.y() - o.y()) / dots_per_mm_y());
  d.elements = S57::ElementData(GL_TRIANGLES, indices.size() * sizeof(GLuint), 6);


  const GLuint ioff = vertices.size() / 4;
  indices << 0 + ioff << 1 + ioff << 2 + ioff << 0 + ioff << 2 + ioff << 3 + ioff;

  // upper left
  const QPointF p0(0., 0.);
  // lower right
  const QPointF p1 = p0 + QPointF(w / dots_per_mm_x(), - h / dots_per_mm_y());
  const QPointF t1 = t0 + QPointF(w / W, h / H);

  vertices << p0.x() << p0.y() << t0.x() << t0.y();
  vertices << p1.x() << p0.y() << t1.x() << t0.y();
  vertices << p1.x() << p1.y() << t1.x() << t1.y();
  vertices << p0.x() << p1.y() << t0.x() << t1.y();

}

bool RasterSymbolManager::paintIcon(QPainter& painter, quint32 index, S52::SymbolType type) {
  const SymbolKey key(index, type);
  if (!m_pixmapCache.contains(key)) {
    if (!m_painterData.contains(key)) return false;
    auto pix = new QPixmap;
    int h = 2 * m_painterData[key].graphicsLocation.height();
    *pix = QPixmap(m_symbolAtlas).copy(m_painterData[key].graphicsLocation).scaledToHeight(h);
    m_pixmapCache.insert(key, pix);
  }
  const auto ox = qMax(0, painter.device()->width() / 6 + m_painterData[key].offset.x());
  const auto oy = qMax(0, painter.device()->height() / 6 + m_painterData[key].offset.y());
  painter.drawPixmap(ox, oy, *m_pixmapCache[key]);
  return true;
}

