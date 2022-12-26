/* -*- coding: utf-8-unix -*-
 *
 * File: src/rastersymbolmanager.h
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


class QOpenGLTexture;
class QXmlStreamReader;
class QPainter;


class RasterSymbolManager: public QObject {

  Q_OBJECT

public:

  RasterSymbolManager();

  static RasterSymbolManager* instance();
  void init();

  SymbolData symbolData(quint32 index, S52::SymbolType type) const;
  SymbolData symbolData(const SymbolKey& key) const;
  bool paintIcon(PickIconData& icon, quint32 index, S52::SymbolType type, bool centerd = false);
  void changeSymbolAtlas();

  void initializeGL();
  void finalizeGL();


  void bind();

  ~RasterSymbolManager();

private:

  void createSymbols();


  struct ParseData {
    QPointF offset;
    QSizeF size;
    qreal minDist;
    qreal maxDist;
    S57::ElementData elements;
    QRect graphicsLocation;
    QPoint graphicsOffset;
  };

  struct PainterData {
    PainterData(const QRect& g, const QPoint& o)
      : graphicsLocation(g)
      , offset(o) {}
    PainterData() = default;

    QRect graphicsLocation;
    QPoint offset;
  };

  using SymbolMap = QHash<SymbolKey, SymbolData>;
  using PainterDataMap = QHash<SymbolKey, PainterData>;
  using PixmapCache = QCache<SymbolKey, QPixmap>;

  void parseSymbols(QXmlStreamReader& reader, S52::SymbolType type);

  void parseSymbolData(QXmlStreamReader& reader, ParseData& d);

  SymbolMap m_symbolMap;
  SymbolData m_invalid;
  QOpenGLBuffer m_coordBuffer;
  QOpenGLBuffer m_indexBuffer;
  QOpenGLTexture* m_symbolTexture;
  QString m_symbolAtlas;
  QSize m_size;
  // paintIcon interface
  PainterDataMap m_painterData;
  PixmapCache m_pixmapCache;
  // backup
  GL::VertexVector m_vertexBackup;
  GL::IndexVector m_indexBackup;
};
