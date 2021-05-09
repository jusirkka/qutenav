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

ChartCover::ChartCover(const LLPolygon& cov, const LLPolygon& nocov,
                       const WGS84Point& sw, const WGS84Point& ne,
                       const GeoProjection* proj)
  : m_ref(proj->reference())
  , m_cover() {

  const auto ll = proj->fromWGS84(sw);
  const auto ur = proj->fromWGS84(ne);
  QRectF box(ll, ur);

  // offset bbox to account boundary not being part of polygon in inpolygon
  QRectF bbox(ll + QPointF(1., 1.), ur  - QPointF(1., 1.));

  if (cov.isEmpty()) {
    qCDebug(CMGR) << "KV::Region" << box;
    m_cover = KV::Region(box);
  } else {
    for (const WGS84PointVector& vs: cov) {
      PointVector ps;
      for (auto v: vs) {
        ps << proj->fromWGS84(v);
      }
      m_cover += approximate(ps, bbox);
    }
  }


  for (const WGS84PointVector& vs: nocov) {
    PointVector ps;
    for (auto v: vs) {
      ps << proj->fromWGS84(v);
    }
    m_cover -= approximate(ps, bbox);
  }
}

KV::Region ChartCover::region(const GeoProjection *gp) const {
  return m_cover.translated(gp->fromWGS84(m_ref));
}


// https://wrf.ecse.rpi.edu//Research/Short_Notes/pnpoly.html
static bool inpolygon(const PointVector& ps, qreal x, qreal y) {
  bool c = false;
  const int n = ps.size();
  for (int i = 0, j = n - 1; i < n; j = i++) {
    if (((ps[i].y() > y) != (ps[j].y() > y)) &&
        (x < (ps[j].x() - ps[i].x()) * (y - ps[i].y()) / (ps[j].y() - ps[i].y()) + ps[i].x())) {
      c = !c;
    }
  }
  return c;
}

KV::Region ChartCover::approximate(const PointVector& poly, const QRectF& box) const {

  const qreal dx = box.width() / (gridWidth - 1);
  const qreal dy = box.height() / (gridWidth - 1);

  QVector<bool> grid(gridWidth * gridWidth);

  for (int i = 0; i < gridWidth; i++) {
    const auto x = box.left() + i * dx;
    for (int j = 0; j < gridWidth; j++) {
      const auto y = box.top() + j * dy;
      grid[i * gridWidth + j] = inpolygon(poly, x, y);
    }
  }

  KV::Region reg;

  for (int i = 0; i < gridWidth - 1; i++) {
    const auto x = box.left() + i * dx;
    for (int j = 0; j < gridWidth - 1; j++) {
      const auto y = box.top() + j * dy;
      if (grid[i * gridWidth + j] || grid[(i + 1) * gridWidth + j] ||
          grid[i * gridWidth + j + 1] || grid[(i + 1) * gridWidth + j + 1]) {
        reg += QRectF(x, y, dx, dy);
      }
    }
  }

  return reg;
}


// Note: not 100% reliable, but good enough (should test x/y is constant alternatingly)
bool ChartCover::isRectangle(const PointVector& poly) const {
  if (poly.size() != 4) return false;

  const qreal eps = 1.e-10;
  for (int k = 0; k < 4; k++) {
    const auto p1 = poly[k];
    const auto p2 = poly[(k + 1) % 4];
    if (std::abs(p2.x() - p1.x()) > eps && std::abs(p2.y() - p1.y()) > eps) return false;
  }

  return true;
}
