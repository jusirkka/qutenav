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
  , m_size(QImage(S52::GetRasterFileName()).size())
  , m_pixmapCache(100 * sizeof(QPixmap))
{}

void RasterSymbolManager::init() {
  qCDebug(CDPY) << "init (noop)";
}

void RasterSymbolManager::initializeGL() {
  changeSymbolAtlas();
  createSymbols();
}

void RasterSymbolManager::finalizeGL() {
  m_coordBuffer.bind();
  m_vertexBackup.resize(m_coordBuffer.size() / sizeof(GLfloat));
  memcpy(m_vertexBackup.data(), m_coordBuffer.map(QOpenGLBuffer::ReadOnly), m_coordBuffer.size());
  m_coordBuffer.unmap();
  m_coordBuffer.destroy();

  m_indexBuffer.bind();
  m_indexBackup.resize(m_indexBuffer.size() / sizeof(GLuint));
  memcpy(m_indexBackup.data(), m_indexBuffer.map(QOpenGLBuffer::ReadOnly), m_indexBuffer.size());
  m_indexBuffer.unmap();
  m_indexBuffer.destroy();

  delete m_symbolTexture;
  m_symbolTexture = nullptr;
}

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
  if (rname == m_symbolAtlas && m_symbolTexture != nullptr) return;
  m_symbolAtlas = rname;
  QImage img(m_symbolAtlas);
  m_size = img.size();
  delete m_symbolTexture;
  qCDebug(CDPY) << "new texture";
  m_symbolTexture = new QOpenGLTexture(img, QOpenGLTexture::DontGenerateMipMaps);
  m_symbolTexture->setWrapMode(QOpenGLTexture::ClampToEdge);
  m_symbolTexture->setMinMagFilters(QOpenGLTexture::Linear, QOpenGLTexture::Linear);
}

void RasterSymbolManager::createSymbols() {

  if (m_coordBuffer.isCreated()) return;

  if (m_vertexBackup.isEmpty()) {
    QFile file(S52::FindPath("chartsymbols.xml"));
    file.open(QFile::ReadOnly);
    QXmlStreamReader reader(&file);

    reader.readNextStartElement();
    Q_ASSERT(reader.name() == "chartsymbols");

    // Note: there exists no raster line styles
    while (reader.readNextStartElement()) {
      if (reader.name() == "patterns") {
        parseSymbols(reader, S52::SymbolType::Pattern);
      } else if (reader.name() == "symbols") {
        parseSymbols(reader, S52::SymbolType::Single);
      } else {
        reader.skipCurrentElement();
      }
    }
    file.close();
  }

  // fill in vertex buffer
  m_coordBuffer.create();
  m_coordBuffer.bind();
  m_coordBuffer.setUsagePattern(QOpenGLBuffer::StaticDraw);
  m_coordBuffer.allocate(m_vertexBackup.constData(), sizeof(GLfloat) * m_vertexBackup.size());
  m_vertexBackup.clear();

  // fill in index buffer
  m_indexBuffer.create();
  m_indexBuffer.bind();
  m_indexBuffer.setUsagePattern(QOpenGLBuffer::StaticDraw);
  m_indexBuffer.allocate(m_indexBackup.constData(), sizeof(GLuint) * m_indexBackup.size());
  m_indexBackup.clear();
}

void RasterSymbolManager::parseSymbols(QXmlStreamReader &reader,
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
        parseSymbolData(reader, d);
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
                                          ParseData &d) {
  Q_ASSERT(reader.name() == "bitmap");

  int w = reader.attributes().value("width").toInt();
  int h = reader.attributes().value("height").toInt();
  d.size = QSizeF(w / dots_per_mm_x(), h  / dots_per_mm_y());

  QPoint p;
  QPoint o;

  QPointF t0;
  const qreal W = m_size.width();
  const qreal H = m_size.height();

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
  d.elements = S57::ElementData(GL_TRIANGLES, m_indexBackup.size() * sizeof(GLuint), 6);


  const GLuint ioff = m_vertexBackup.size() / 4;
  m_indexBackup << 0 + ioff << 1 + ioff << 2 + ioff << 0 + ioff << 2 + ioff << 3 + ioff;

  // upper left
  const QPointF p0(0., 0.);
  // lower right
  const QPointF p1 = p0 + QPointF(w / dots_per_mm_x(), - h / dots_per_mm_y());
  const QPointF t1 = t0 + QPointF(w / W, h / H);

  m_vertexBackup << p0.x() << p0.y() << t0.x() << t0.y();
  m_vertexBackup << p1.x() << p0.y() << t1.x() << t0.y();
  m_vertexBackup << p1.x() << p1.y() << t1.x() << t1.y();
  m_vertexBackup << p0.x() << p1.y() << t0.x() << t1.y();

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

