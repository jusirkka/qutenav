/* -*- coding: utf-8-unix -*-
 *
 * File: src/geoprojection.h
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
#include <QSizeF>

class GeoProjection {
public:

  static GeoProjection* CreateProjection(const QString& className);

  virtual WGS84Point toWGS84(const QPointF& p) const = 0;
  virtual QPointF fromWGS84(const WGS84Point& w) const = 0;
  virtual QString className() const = 0;

  virtual void setReference(const WGS84Point& w) {m_ref = w;}
  const WGS84Point& reference() const {return m_ref;}

  void setScaling(const QSizeF s) {m_scaling = s;}
  const QSizeF& scaling() const {return m_scaling;}

  GeoProjection()
    : m_ref(WGS84Point::fromLL(0., 0.))
    , m_scaling(1., 1.) {}

  virtual ~GeoProjection() = default;

protected:

  WGS84Point m_ref;
  QSizeF m_scaling;

};

class SimpleMercator: public GeoProjection {
public:
  SimpleMercator();
  WGS84Point toWGS84(const QPointF& p) const override;
  QPointF fromWGS84(const WGS84Point& p) const override;
  QString className() const {return "SimpleMercator";}
  void setReference(const WGS84Point& w) override;


private:

  static constexpr double z0 = WGS84Point::semimajor_axis * 0.9996;

  double m_y30;

};


class CM93Mercator: public GeoProjection {
public:
  CM93Mercator();
  WGS84Point toWGS84(const QPointF& p) const override;
  QPointF fromWGS84(const WGS84Point& p) const override;
  QString className() const {return "CM93Mercator";}
  void setReference(const WGS84Point& w) override;
  void setReference(const QPointF& p);

  // From opencpn / cm93.h
  static constexpr double zC = 6378388.0;

  static constexpr double scale = WGS84Point::semimajor_axis * 0.9996 / zC;

private:


  double m_y30;

};


bool operator!= (const GeoProjection& p1, const GeoProjection& p2);
bool operator== (const GeoProjection& p1, const GeoProjection& p2);

QRectF findBoundingBox(const GeoProjection* p, const WGS84PointVector& points);
