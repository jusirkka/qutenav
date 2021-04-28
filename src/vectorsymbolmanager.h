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


class QXmlStreamReader;


class VectorSymbolManager {

public:

  VectorSymbolManager();

  static VectorSymbolManager* instance();
  void createSymbols();

  SymbolData symbolData(quint32 index, S52::SymbolType type) const;

  void bind();

  ~VectorSymbolManager();


private:

  // length units in millimeters
  static const inline qreal mmUnit = .01;

  struct ParseData {
    QPointF offset;
    QPointF pivot;
    QSizeF size;
    qreal minDist;
    qreal maxDist;
  };

  using SymbolMap = QHash<SymbolKey, SymbolData>;

  void parseSymbols(QXmlStreamReader& reader,
                    GL::VertexVector& vertices,
                    GL::IndexVector& indices,
                    S52::SymbolType type);

  void parseSymbolData(QXmlStreamReader& reader,
                       ParseData& d,
                       QString& src);

  SymbolMap m_symbolMap;
  SymbolData m_invalid;
  QOpenGLBuffer m_coordBuffer;
  QOpenGLBuffer m_indexBuffer;
  const QStringList m_blacklist;
};
