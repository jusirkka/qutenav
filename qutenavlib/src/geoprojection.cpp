/* -*- coding: utf-8-unix -*-
 *
 * File: src/geoprojection.cpp
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
#include "geoprojection.h"
#include <QVector>
#include <QRectF>

GeoProjection* GeoProjection::CreateProjection(const QString& className) {
  if (className == "SimpleMercator") {
    return new SimpleMercator;
  }
  if (className == "CM93Mercator") {
    return new CM93Mercator;
  }
  if (className == "NoopProjection") {
    return new NoopProjection;
  }
  return nullptr;
}

SimpleMercator::SimpleMercator()
  : GeoProjection()
{}

void SimpleMercator::setReference(const WGS84Point &w) {
  GeoProjection::setReference(w);
  const double s = sin(m_ref.radiansLat());
  m_y30 = .5 * log((1 + s) / (1 - s)) * z0;
}

QPointF SimpleMercator::fromWGS84(const WGS84Point &p) const {
  const double s = sin(p.radiansLat());
  const double y3 = .5 * log((1 + s) / (1 - s)) * z0;

  const auto d = Angle::fromDegrees(p.lng() - m_ref.lng());

  return QPointF(d.radians * z0, y3 - m_y30);
}

WGS84Point SimpleMercator::toWGS84(const QPointF &p) const {
  return WGS84Point::fromLLRadians(m_ref.radiansLng() + p.x() / z0,
                                   2.0 * atan(exp((m_y30 + p.y()) / z0)) - M_PI_2);
}


CM93Mercator::CM93Mercator()
  : GeoProjection()
{}

void CM93Mercator::setReference(const WGS84Point& w) {
  GeoProjection::setReference(w);
  const double s = sin(m_ref.radiansLat());
  m_y30 = .5 * log((1 + s) / (1 - s)) * zC;
}

void CM93Mercator::setReference(const QPointF& p) {
  GeoProjection::setReference(WGS84Point::fromLL(0., 0.));
  QSizeF sc = scaling();
  setScaling(QSizeF(1., 1.));
  GeoProjection::setReference(toWGS84(p));
  setScaling(sc);
  const double s = sin(m_ref.radiansLat());
  m_y30 = .5 * log((1 + s) / (1 - s)) * zC;
}

QPointF CM93Mercator::fromWGS84(const WGS84Point& p) const {
  const double s = sin(p.radiansLat());
  const double y3 = .5 * log((1 + s) / (1 - s)) * zC;

  const auto d = Angle::fromDegrees(p.lng() - m_ref.lng());

  return QPointF(d.radians * zC / m_scaling.width(), (y3 - m_y30) / m_scaling.height());
}

WGS84Point CM93Mercator::toWGS84(const QPointF &p) const {
  return WGS84Point::fromLLRadians(
        m_ref.radiansLng() + p.x() * m_scaling.width() / zC,
        2.0 * atan(exp((m_y30 + p.y() * m_scaling.height()) / zC)) - M_PI_2);
}


NoopProjection::NoopProjection()
  : GeoProjection()
{}

QPointF NoopProjection::fromWGS84(const WGS84Point &p) const {
  return QPointF(p.lng(), p.lat());
}

WGS84Point NoopProjection::toWGS84(const QPointF &p) const {
  return WGS84Point::fromLL(p.x(), p.y());
}

bool operator!= (const GeoProjection& p1, const GeoProjection& p2) {
  return p1.className() != p2.className();
}

bool operator== (const GeoProjection& p1, const GeoProjection& p2) {
  return !(p1 != p2);
}


QRectF findBoundingBox(const GeoProjection *p, const WGS84PointVector& points) {
  QPointF ur(-1.e15, -1.e15);
  QPointF ll(1.e15, 1.e15);
  for (const WGS84Point& point: points) {
    QPointF q = p->fromWGS84(point);
    ur.setX(qMax(ur.x(), q.x()));
    ur.setY(qMax(ur.y(), q.y()));
    ll.setX(qMin(ll.x(), q.x()));
    ll.setY(qMin(ll.y(), q.y()));
  }
  return QRectF(ll, ur); // inverted y-axis
}
