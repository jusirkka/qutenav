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

#include "region.h"
#include "region_p.h"
#include <QRectF>
#include "geoprojection.h"
#include "logging.h"

using namespace KV;

Region::Region()
  : d(new RegionPrivate)
{}

Region::Region(const Region& r)
  : d(r.d)
{}

Region::Region(const QRectF& r)
  : d(new RegionPrivate) {
  d->region = ToRect(r);
}

Region &Region::operator=(const Region &r) {
  d = r.d;
  return *this;
}

Region::~Region() {}


bool Region::isEmpty() const {
  return d->region.isEmpty();
}

bool Region::contains(const QRectF &r) const {
  return d->region.contains(ToRect(r));
}

bool Region::intersects(const QRectF& rect) const {
  return d->region.intersects(ToRect(rect));
}

Region Region::intersected(const Region &r) const {
  Region ret;
  ret.d->region = d->region.intersected(r.d->region);
  return ret;
}


Region Region::intersected(const QRectF& r) const {
  return intersected(Region(r));
}

void Region::translate(const QPointF& p) {
  d->region.translate(ToPoint(p));
}

Region Region::translated(const QPointF& p) const {
  Region ret(*this);
  ret.translate(p);
  return ret;
}

Region& Region::operator+=(const Region& r) {
  d->region += r.d->region;
  return *this;
}

Region& Region::operator-=(const Region& r) {
  d->region -= r.d->region;
  return *this;
}

Region& Region::operator+=(const QRectF& r) {
  d->region += ToRect(r);
  return *this;
}

QRectF Region::boundingRect() const noexcept {
  return ToRectF(d->region.boundingRect());
}

qreal Region::area() const {
  const auto rs = rects();
  return std::accumulate(rs.begin(), rs.end(), 0., [] (qreal s, const QRectF& r) {
    return s + r.width() * r.height();
  });
}

QVector<QRectF> Region::rects() const {
  const auto rs = d->region.rects();
  QVector<QRectF> rfs;
  for (const auto& r: rs) {
    rfs.append(ToRectF(r));
  }
  return rfs;
}

int Region::rectCount() const noexcept {
  return d->region.rectCount();
}


WGS84PointVector Region::toWGS84(const GeoProjection* gp) const {
  WGS84PointVector ps;
  const auto rs = rects();
  for (const QRectF& r: rs) {
    if (r.width() * r.height() < 1.) {
      // qCWarning(CREG) << "Bad rectangle in region" << r;
      continue;
    }
    ps << gp->toWGS84(r.topLeft()) << gp->toWGS84(r.bottomRight());
  }
  return ps;
}

Region::Region(const WGS84PointVector& cs, const GeoProjection* gp) {
  Region s;
  const int n = cs.size() / 2;
  for (int i = 0; i < n; i++) {
    // sw, ne
    const auto r = QRectF(gp->fromWGS84(cs[2 * i]), gp->fromWGS84(cs[2 * i + 1]));
    if (r.isEmpty()) {
      qCWarning(CREG) << "Empty rectangle" << r;
      continue;
    }
    s += r;
  }
  d = s.d;
}
