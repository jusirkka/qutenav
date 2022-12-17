/* -*- coding: utf-8-unix -*-
 *
 * File: src/vectorsymbolmanager.h
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
#pragma once

#include "types.h"
#include "s57object.h"
#include <QRect>
#include <QHash>
#include <QOpenGLBuffer>
#include "symboldata.h"
#include <QCache>


class QXmlStreamReader;
class QPainter;

struct CacheKey {

  CacheKey(quint32 idx, S52::SymbolType s, qint16 a)
    : index(idx)
    , type(s)
    , angle(a) {}

  CacheKey() = default;

  SymbolKey key() const {
    return SymbolKey(index, type);
  }

  quint32 index;
  S52::SymbolType type;
  qint16 angle;
};

inline bool operator== (const CacheKey& k1, const CacheKey& k2) {
  if (k1.index != k2.index) return false;
  if (k1.type != k2.type) return false;
  return k1.angle == k2.angle;
}

inline uint qHash(const CacheKey& key) {
  return qHash(qMakePair(key.index, 360 * as_numeric(key.type) + 180 + key.angle));
}


class VectorSymbolManager {

public:

  VectorSymbolManager();

  static VectorSymbolManager* instance();

  void init();

  SymbolData symbolData(quint32 index, S52::SymbolType type) const;
  SymbolData symbolData(const SymbolKey& key) const;
  bool paintIcon(QPainter& painter, quint32 index, S52::SymbolType type, qint16 angle);

  void initializeGL();
  void finalizeGL();

  void bind();

  ~VectorSymbolManager();


private:

  void createSymbols();

  // length units in millimeters
  static const inline qreal mmUnit = .01;

  struct ParseData {
    QPointF offset;
    QPointF pivot;
    QSizeF size;
    qreal minDist;
    qreal maxDist;
    QPoint center;
  };

  struct PainterData {
    PainterData(const QString& s, const QString& c, const QPoint& p)
      : src(s)
      , cmap(c)
      , center(p) {}
    PainterData() = default;

    QString src;
    QString cmap;
    QPoint center;
  };


  using SymbolMap = QHash<SymbolKey, SymbolData>;
  using PainterDataMap = QHash<SymbolKey, PainterData>;
  using PixmapCache = QCache<CacheKey, QPixmap>;

  void parseSymbols(QXmlStreamReader& reader, S52::SymbolType type);
  void parseSymbolData(QXmlStreamReader& reader, ParseData& d, QString& src);

  SymbolMap m_symbolMap;
  SymbolData m_invalid;
  QOpenGLBuffer m_coordBuffer;
  QOpenGLBuffer m_indexBuffer;
  // paintIcon interface
  PainterDataMap m_painterData;
  PixmapCache m_pixmapCache;
  // backup
  GL::VertexVector m_vertexBackup;
  GL::IndexVector m_indexBackup;
};
