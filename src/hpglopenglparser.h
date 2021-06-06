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

namespace HPGL {

class OpenGLParser: public Parser {

public:

  OpenGLParser(const QString& src, const QString& colors, const QPointF& pivot);

  struct Data {
    S52::Color color;
    GL::IndexVector indices;
    GL::VertexVector vertices;
  };

  using DataVector = QVector<Data>;

  const DataVector& data() const {return m_data;}

private:

  using DataHash = QHash<S52::Color, Data>;
  using DataIterator = QHash<S52::Color, Data>::iterator;

  DataVector m_data;


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

  using PointList = QVector<QPointF>;

  struct LineString {
    PointList points;
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

  void triangulate(const PointList& points, Data& out);
  void thickerlines(const LineString& ls, qreal lw, Data& out);
  static void mergeData(Data& tgt, const Data& d);
  QPointF makePoint(int x, int y) const;

  Sketch m_currentSketch;
  QStack<Sketch> m_sketches;

  bool m_started;
  bool m_penDown;

  QPointF m_pen;
  QPointF m_pivot;

  DataHash m_storage;

};
}



