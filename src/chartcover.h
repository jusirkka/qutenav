/* -*- coding: utf-8-unix -*-
 *
 * File: src/chartcover.h
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
#include <QPointF>
#include <QVector>
#include "region.h"


using PointVector = QVector<QPointF>;
using Polygon = QVector<PointVector>;

class GeoProjection;

class ChartCover {
public:

  ChartCover(const ChartCover&);
  ChartCover();
  ChartCover operator =(const ChartCover&);
  ChartCover(const WGS84Polygon& cov, const WGS84Polygon& nocov,
             const WGS84Point& sw, const WGS84Point& ne,
             const GeoProjection* gp);

  KV::Region region(const GeoProjection* gp) const;
  const WGS84Polygon& coverage() const {return m_cov;}
  const WGS84Polygon& nocoverage() const {return m_nocov;}

private:

  static const int gridWidth = 21;

  KV::Region approximate(const PointVector& poly, const QRectF& box, bool inner = false) const;
  bool isRectangle(const PointVector& poly) const;

  WGS84Point m_ref;
  KV::Region m_cover;
  const WGS84Polygon m_cov;
  const WGS84Polygon m_nocov;
};
