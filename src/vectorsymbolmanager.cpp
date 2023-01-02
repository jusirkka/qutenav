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
#include "platform.h"
#include "settings.h"
#include <QPixmap>

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

void VectorSymbolManager::initializeGL() {
  createSymbols();
}

void VectorSymbolManager::finalizeGL() {
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
}


VectorSymbolManager::~VectorSymbolManager() {}

SymbolData VectorSymbolManager::symbolData(quint32 index, S52::SymbolType type) const {
  return symbolData(SymbolKey(index, type));
}

SymbolData VectorSymbolManager::symbolData(const SymbolKey& key) const {
  if (m_symbolMap.contains(key)) return m_symbolMap[key];
  return m_invalid;
}

void VectorSymbolManager::bind() {
  m_indexBuffer.bind();
  m_coordBuffer.bind();
}


void VectorSymbolManager::createSymbols() {

  if (m_coordBuffer.isCreated()) return;

  if (m_vertexBackup.isEmpty()) {

    QFile file(S52::FindPath("chartsymbols.xml"));
    file.open(QFile::ReadOnly);
    QXmlStreamReader reader(&file);

    reader.readNextStartElement();
    Q_ASSERT(reader.name() == "chartsymbols");

    while (reader.readNextStartElement()) {
      if (reader.name() == "line-styles") {
        parseSymbols(reader, S52::SymbolType::LineStyle);
      } else if (reader.name() == "patterns") {
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


void VectorSymbolManager::parseSymbols(QXmlStreamReader& reader, S52::SymbolType t) {
  const QString itemName = t == S52::SymbolType::Single ?
        "symbol" : t == S52::SymbolType::LineStyle ? "line-style" : "pattern";

  while (reader.readNextStartElement()) {
    Q_ASSERT(reader.name() == itemName);

    QString symbolName;
    ParseData d;
    bool staggered = false;

    while (reader.readNextStartElement()) {
      if (reader.name() == "name") {
        symbolName = reader.readElementText();
      } else if (reader.name() == "vector") {
        parseSymbolData(reader, d);
      } else if (reader.name() == "HPGL") {
        d.pdata.src = reader.readElementText();
      } else if (reader.name() == "color-ref") {
        d.pdata.cmap = reader.readElementText();
      } else if (reader.name() == "filltype") {
        staggered = reader.readElementText() != "L";
      } else {
        reader.skipCurrentElement();
      }
    }

    if (!d.size.isValid()) continue;

    HPGL::OpenGLParser parser(d.pdata.src, d.pdata.cmap, d.pivot);
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
      e.offset = m_indexBackup.size() * sizeof(GLuint);
      elems.append(e);
      // update vertices & indices
      const GLuint offset = m_vertexBackup.size() / 2;
      m_vertexBackup.append(item.vertices);
      for (GLuint i: item.indices) {
        m_indexBackup << offset + i;
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
    m_painterData.insert(key, d.pdata);
  }
}

void VectorSymbolManager::parseSymbolData(QXmlStreamReader &reader, ParseData &d) {
  Q_ASSERT(reader.name() == "vector");

  int w = reader.attributes().value("width").toInt();
  int h = reader.attributes().value("height").toInt();
  d.size = QSizeF(mmUnit * w, mmUnit * h);

  QPointF o;

  auto scalePoint = [] (const QPointF& p) {
    return QPointF(p.x() * Platform::dots_per_mm_x(), p.y() * Platform::dots_per_mm_y());
  };

  while (reader.readNextStartElement()) {
    if (reader.name() == "distance") {
      d.minDist = reader.attributes().value("min").toInt() * mmUnit;
      d.maxDist = reader.attributes().value("max").toInt() * mmUnit;
      reader.skipCurrentElement();
    } else if (reader.name() == "pivot") {
      d.pivot = QPointF(reader.attributes().value("x").toInt() * mmUnit,
                        reader.attributes().value("y").toInt() * mmUnit);
      d.pdata.pivot = scalePoint(d.pivot);
      reader.skipCurrentElement();
    } else if (reader.name() == "origin") {
      o = QPointF(reader.attributes().value("x").toInt() * mmUnit,
                  reader.attributes().value("y").toInt() * mmUnit);
      reader.skipCurrentElement();
    } else if (reader.name() == "HPGL") {
      d.pdata.src = reader.readElementText();
    } else {
      reader.skipCurrentElement();
    }
  }

  // offset of the upper left corner
  d.offset = QPointF(o.x() - d.pivot.x(), d.pivot.y() - o.y());
}

QPixmap VectorSymbolManager::getPixmap(const CacheKey& key) {
  if (!m_pixmapCache.contains(key)) {
    const SymbolKey key0 = key.key();
    if (!m_painterData.contains(key0)) return QPixmap();
    auto pix = new QPixmap;
    PainterData& d = m_painterData[key0];
    if (key.angle == lcAreaAngle || key.angle == lcLineAngle) {
      HPGL::PixmapParser parser(d.src, d.cmap, d.pivot, key.angle == lcLineAngle);
      *pix = parser.pixmap();
      d.bbox = parser.bbox();
    } else {
      HPGL::PixmapParser parser(d.src, d.cmap, d.pivot, key.angle);
      *pix = parser.pixmap();
      d.bbox = parser.bbox();
    }
    m_pixmapCache.insert(key, pix);
  }
  return *m_pixmapCache[key];
}

bool VectorSymbolManager::paintIcon(PickIconData& icon,
                                    quint32 index, S52::SymbolType type,
                                    qint16 angle, bool opt) {
  QPainter painter(&icon.canvas);

  if (type == S52::SymbolType::LineStyle) {

    const CacheKey key(index, type, opt ? lcLineAngle : lcAreaAngle);
    auto pix = getPixmap(key);
    if (pix.isNull()) return false;

    QPointF c = .5 * QPointF(icon.canvas.width() - pix.width(), icon.canvas.height() - pix.height());
    painter.drawPixmap(c, pix);
    icon.bbox |= QRectF(c, pix.size());

    return true;
  }

  const CacheKey key(index, type, angle);
  auto pix = getPixmap(key);
  if (pix.isNull()) return false;

  if (type == S52::SymbolType::Pattern) {
    const QSizeF s(Platform::pick_icon_size(), Platform::pick_icon_size());
    const QPointF c = .5 * QPointF(icon.canvas.width() - s.width(), icon.canvas.height() - s.height());
    const QRectF box(c, s);
    icon.bbox |= box;

    const auto adv = m_symbolMap[key.key()].advance();
    const qreal X = adv.x * Platform::dots_per_mm_x() / Platform::display_length_scaling();
    const qreal Y = adv.xy.y() * Platform::dots_per_mm_y() / Platform::display_length_scaling();
    const qreal xs = adv.xy.x() * Platform::dots_per_mm_x() / Platform::display_length_scaling();

    const int ny = std::floor(box.top() / Y);
    const int my = std::ceil(box.bottom() / Y) + 1;
    for (int ky = ny; ky < my; ky++) {
      const qreal x1 = ky % 2 == 0 ? 0. : xs;
      const int nx = std::floor((box.left() - x1) / X);
      const int mx = std::ceil((box.right() - x1) / X) + 1;
      for (int kx = nx; kx < mx; kx++) {
        painter.drawPixmap(kx * X + x1, ky * Y, pix);
      }
    }

    return true;
  }
  // type == S52::SymbolType::Single
  if (opt) { // opt = centered for type = Single

    if (pix.width() < Platform::pick_icon_min_size() && pix.width() >= pix.height()) {
      pix = pix.scaledToWidth(Platform::pick_icon_min_size());
    } else if (pix.height() < Platform::pick_icon_min_size() && pix.height() >= pix.width()) {
      pix = pix.scaledToHeight(Platform::pick_icon_min_size());
    } else if (pix.width() > Platform::pick_icon_max_size() && pix.width() >= pix.height()) {
      pix = pix.scaledToWidth(Platform::pick_icon_max_size());
    } else if (pix.height() > Platform::pick_icon_max_size() && pix.height() >= pix.width()) {
      pix = pix.scaledToHeight(Platform::pick_icon_max_size());
    }

    QPointF c = .5 * QPointF(icon.canvas.width() - pix.width(), icon.canvas.height() - pix.height());
    painter.drawPixmap(c, pix);
    icon.bbox |= QRectF(c, pix.size());
  } else {
    const auto p = m_painterData[key.key()].bbox.topLeft();
    const auto ox = painter.device()->width() / 2 - p.x();
    const auto oy = painter.device()->height() / 2 - p.y();
    painter.drawPixmap(ox, oy, pix);
    icon.bbox |= QRectF(ox, oy, pix.width(), pix.height());
  }
  return true;
}
