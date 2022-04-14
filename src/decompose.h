/* -*- coding: utf-8-unix -*-
 *
 * File: src/decompose.h
 *
 * Copyright (C) 2022 Jukka Sirkka
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

#include <QVector>
#include <QPointF>
#include <QRectF>

namespace KV {

using XVector = QVector<qreal>;

struct Strip {
  qreal y;
  XVector xs;
};

using StripVector = QVector<Strip>;

using PointVector = QVector<QPointF>;
using RectVector = QVector<QRectF>;
using IndexVector = QVector<int>;

class Decomposer {
public:

  Decomposer(const PointVector& poly, qreal prec = .05, bool outer = true);

  QRectF nextRect();

  bool done() const {return m_strips.isEmpty();}

private:

  void find(const QPointF& p, int& ix, int& iy) const;
  void erase(int ix, int iy);
  void insert(int iy, qreal x);

  static IndexVector findDiagonals(const PointVector& ps);
  static PointVector approximateDiagonal(const PointVector& ps, int index, qreal prec, bool outer);
  static void addPoints(PointVector& qs, const PointVector& ps, const PointVector& ds,
                        int i0, int i1);
  static bool reduce(PointVector& qs);

private:

  StripVector m_strips;

};

}
