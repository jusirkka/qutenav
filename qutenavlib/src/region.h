/* -*- coding: utf-8-unix -*-
 *
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

#include <QPointF>
#include <QSharedDataPointer>
#include <QMap>
#include "types.h"

class QRectF;
class QPolygonF;

class GeoProjection;

namespace KV {

class RegionPrivate;

class Region {
public:

  Region();
  Region(qreal x, qreal y, qreal w, qreal h);
  Region(const QRectF& r);
  Region(const Region& region);

  Region(const WGS84PointVector& cs, const GeoProjection* gp);

  ~Region();

  Region& operator=(const Region&);

  inline Region& operator=(Region&& other) noexcept {
    qSwap(d, other.d); return *this;
  }

  WGS84PointVector toWGS84(const GeoProjection* gp) const;

  bool contains(const QPointF& p) const;
  bool contains(const QRectF& r) const;

  void translate(qreal dx, qreal dy) {translate(QPointF(dx, dy));}
  void translate(const QPointF &p);

  Region translated(qreal dx, qreal dy) const {return translated(QPointF(dx, dy));}
  Region translated(const QPointF &p) const;

  bool intersects(const Region& r) const;
  bool intersects(const QRectF& r) const;

  qreal area() const;

  Region& operator+=(const Region& r);
  Region& operator-=(const Region& r);
  Region& operator+=(const QRectF& r);

  bool operator==(const Region& r) const;
  inline bool operator!=(const Region& r) const {return !(operator==(r));}

  int rectCount() const noexcept;
  QVector<QRectF> rects() const;

  Region intersected(const QRectF& r) const;
  Region intersected(const Region& r) const;

  bool isEmpty() const;
  QRectF boundingRect() const noexcept;

private:

  QSharedDataPointer<RegionPrivate> d;

};

using RegionMap = QMap<quint32, Region>;

} // namespace KV
