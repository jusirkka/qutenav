/* -*- coding: utf-8-unix -*-
 *
 * region_p.h
 *
 * Created: 2021-03-14 2021 by Jukka Sirkka
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


#include <QSharedData>
#include <QRectF>
#include <QRegion>
#include <climits>
#include <cmath>

namespace KV {


struct RegionPrivate: public QSharedData {

  QRegion region;

};

static const qreal factor = 100.;

QRect ToRect(const QRectF& r) {

  const qreal x0 = std::floor(r.left() * factor);
  const qreal x1 = std::ceil(r.right() * factor);
  const qreal y0 = std::floor(r.top() * factor);
  const qreal y1 = std::ceil(r.bottom() * factor);

  Q_ASSERT(x0 > INT_MIN && x1 < INT_MAX && y0 > INT_MIN && y1 < INT_MAX);

  return QRect(QPoint(x0, y0), QPoint(x1, y1));
}

QRectF ToRectF(const QRect& r) {

  const QPoint p0 = r.topLeft();
  const QPoint p1 = r.bottomRight();

  return QRectF(QPointF(p0.x(), p0.y()) / factor, QPointF(p1.x(), p1.y()) / factor);
}

QPoint ToPoint(const QPointF& p) {
  const qreal x0 = std::round(p.x() * factor);
  const qreal y0 = std::round(p.y() * factor);

  Q_ASSERT(x0 > INT_MIN && x0 < INT_MAX && y0 > INT_MIN && y0 < INT_MAX);

  return QPoint(x0, y0);
}


} // namespace KV
