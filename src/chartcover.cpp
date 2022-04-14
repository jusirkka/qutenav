/* -*- coding: utf-8-unix -*-
 *
 * File: src/chartcover.cpp
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
#include "chartcover.h"
#include "geoprojection.h"
#include "logging.h"
#include <QRectF>
#include <functional>
#include "decompose.h"

ChartCover::ChartCover(const WGS84Polygon& cov, const WGS84Polygon& nocov,
                       const WGS84Point& sw, const WGS84Point& ne,
                       const GeoProjection* proj, quint32 scale)
  : m_ref(proj->reference())
  , m_cover()
  , m_cov(cov)
  , m_nocov(nocov) {

  const auto ll = proj->fromWGS84(sw);
  const auto ur = proj->fromWGS84(ne);

  const WGS84Point nw = WGS84Point::fromLL(sw.lng(), ne.lat());
  // 1 cm coarseness at nominal scale
  qreal prec = 0.01 * scale * (ur.y() - ll.y()) / (nw - sw).meters();

  if (cov.isEmpty()) {
    const QRectF box(ll, ur);
    qCDebug(CMGR) << "KV::Region" << box;
    m_cover = KV::Region(box);
  } else {
    for (const WGS84PointVector& vs: cov) {
      PointVector ps;
      for (auto v: vs) {
        ps << proj->fromWGS84(v);
      }
      m_cover += approximate(ps, prec);
    }
  }

  for (const WGS84PointVector& vs: nocov) {
    PointVector ps;
    for (auto v: vs) {
      ps << proj->fromWGS84(v);
    }
    m_cover -= approximate(ps, prec, true);
  }
}

KV::Region ChartCover::region(const GeoProjection *gp) const {
  return m_cover.translated(gp->fromWGS84(m_ref));
}


KV::Region ChartCover::approximate(const PointVector& poly, qreal prec, bool inner) const {
  KV::Region out;
  KV::Decomposer d(poly, prec, !inner);
  while (!d.done()) {
    out += d.nextRect();
  }
  return out;
}


