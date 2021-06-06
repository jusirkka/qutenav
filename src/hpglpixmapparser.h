/* -*- coding: utf-8-unix -*-
 *
 * File: src/hpglparser.h
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

#include "hpglparser.h"
#include <QPixmap>
#include <QPolygonF>
#include <QPainterPath>
#include <QPen>
#include <QBrush>

namespace HPGL {

class PixmapParser: public Parser {

public:

  PixmapParser(const QString& src, const QString& colors, qint16 angle);

  QPixmap pixmap() const {return m_pix;}

private:


  QPixmap m_pix;


private: // bison interface

  // length units to millimeters
  static const inline qreal mmUnit = .01;


  void setColor(char c) override;
  void setAlpha(int a) override;
  void setWidth(int w) override;
  void movePen(const RawPoints& ps) override;
  void drawLineString(const RawPoints& ps) override;
  void drawCircle(int r) override;
  void drawArc(int x, int y, int a) override;
  void pushSketch() override;
  void endSketch() override;
  void fillSketch() override;
  void edgeSketch() override;
  void drawPoint() override;


  struct LineString {
    QPolygonF points;
    bool closed;
  };

  using LineStringList = QVector<LineString>;

  struct Sketch {
    Sketch clone() {
      Sketch s;
      s.color = color;
      s.lineWidth = lineWidth;
      LineString ls;
      ls.closed = false;
      s.parts.append(ls);
      return s;
    }
    S52::Color color;
    qreal lineWidth;
    LineStringList parts;
  };

  QPointF makePoint(int x, int y);

  Sketch m_currentSketch;
  QStack<Sketch> m_sketches;

  bool m_started;
  bool m_penDown;

  QPointF m_penPos;

  using Polygonvector = QVector<QPolygonF>;

  struct PainterItem {
    PainterItem(const QPolygonF& p, const QPen& q)
      : pen(q), brush(), path() {
      path.append(p);
    }
    PainterItem(const QPolygonF& p, const QBrush& b)
      : pen(), brush(b), path() {
      path.append(p);
    }
    PainterItem() = default;

    QPen pen;
    QBrush brush;
    Polygonvector path;
  };

  using PainterItemVector = QVector<PainterItem>;

  PainterItemVector m_items;

};
}



