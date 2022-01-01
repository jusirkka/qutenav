/* -*- coding: utf-8-unix -*-
 *
 * routetracker.cpp
 *
 * Created: 22/04/2021 2021 by Jukka Sirkka
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
#include "routetracker.h"
#include <QDateTime>
#include <QDebug>

using namespace geos::geom;

RouteTracker::RouteTracker(QObject* parent)
  : QObject(parent)
  , m_segmentEndPoint(0)
  , m_segmentBearing(0.)
  , m_segmentETA(0.)
  , m_segmentDTG(0.)
  , m_targetETA(0.)
  , m_targetDTG(0.)
  , m_instant(0.)
  , m_position(0.)
  , m_lineSpeed(0.)
  , m_route()
{}

Coordinate RouteTracker::fromWGS84Point(const WGS84Point &wp) const {
  // Euclidean approximation. TODO replace with Mercator projection
  return Coordinate(z0 * (wp.radiansLng() - m_ref.radiansLng()) * m_c0,
                    z0 * (wp.radiansLat() - m_ref.radiansLat()));
}

void RouteTracker::initialize(const WGS84PointVector& route) {
  if (route.size() > 1) {
    m_ref = route.first();
    m_c0 = cos(m_ref.radiansLat());

    CoordinateSequence coords;
    for (const WGS84Point& wp: route) {
      coords << fromWGS84Point(wp);
    }
    m_route = LineString(coords);
  } else {
    m_route = LineString();
  }

  m_segmentEndPoint = 0;
  emit segmentEndPointChanged();
  m_segmentBearing = undefined;
  emit segmentBearingChanged();

  m_segmentETA = undefined;
  emit segmentETAChanged();
  m_segmentDTG = undefined;
  emit segmentDTGChanged();

  m_targetETA = undefined;
  emit targetETAChanged();
  m_targetDTG = undefined;
  emit targetDTGChanged();

  m_instant = QDateTime::currentMSecsSinceEpoch();
  m_position = undefined;
  m_lineSpeed = 0.;
}

void RouteTracker::update(const WGS84Point& wp, qint64 msecs) {
  if (m_route.isEmpty()) return;
  const auto pos = m_route.position(fromWGS84Point(wp), m_segmentEndPoint);
  // const auto p0 = fromWGS84Point(wp);
  // qDebug() << "pos" << pos << m_route.length() << "(" << p0.x << ", " << p0.y << ")";
  if (!std::isnan(m_position)) {
    const auto speed = (pos - m_position) * 1000. / (msecs - m_instant);
    // qDebug() << "speed" << speed;
    auto count = static_cast<int>((msecs - m_instant) * .001 + .5);
    while (count > 0) {
      m_lineSpeed = alpha * speed + (1. - alpha) * m_lineSpeed;
      // qDebug() << "linespeed" << m_lineSpeed << count;
      count--;
    }
  }
  m_position = pos;
  m_instant = msecs;

  updateSegment();
  updateDTG();
  updateETA();
}

void RouteTracker::updateSegment() {
  if (m_position < 0) {
    if (m_segmentEndPoint > 0) {
      m_segmentEndPoint = 0;
      emit segmentEndPointChanged();
    }
    if (!std::isnan(m_segmentBearing)) {
      m_segmentBearing = undefined;
      emit segmentBearingChanged();
    }
    return;
  }

  if (m_position > m_route.length()) {
    if (m_segmentEndPoint < m_route.size()) {
      m_segmentEndPoint = m_route.size();
      emit segmentEndPointChanged();
    }
    if (!std::isnan(m_segmentBearing)) {
      m_segmentBearing = undefined;
      emit segmentBearingChanged();
    }
    return;
  }

  auto prev = m_segmentEndPoint;
  m_segmentEndPoint = m_route.nextIndex(m_position);
  if (m_segmentEndPoint != prev) {
    emit segmentEndPointChanged();
    if (m_segmentEndPoint >= m_route.size()) {
      m_segmentBearing = undefined;
    } else {
      m_segmentBearing = m_route.segment(m_segmentEndPoint - 1).bearing();
    }
    emit segmentBearingChanged();
  }
}

void RouteTracker::updateDTG() {
  const qreal prevDTG = m_targetDTG;
  const qreal prevSDTG = m_segmentDTG;

  if (m_position > m_route.length()) {
    m_targetDTG = 0.;
    m_segmentDTG = 0.;
  } else {
    m_targetDTG = m_route.length() - m_position;
    m_segmentDTG = m_route.length(m_segmentEndPoint) - m_position;
  }

  // qDebug() << m_targetDTG << m_segmentDTG;

  if (std::isnan(prevDTG)) {
    emit targetDTGChanged();
  } else {
    if (static_cast<int>(m_targetDTG / 100) != static_cast<int>(prevDTG / 100)) {
      emit targetDTGChanged();
    }
  }

  if (std::isnan(prevSDTG)) {
    emit segmentDTGChanged();
  } else {
    if (static_cast<int>(m_segmentDTG / 100) != static_cast<int>(prevSDTG / 100)) {
      emit segmentDTGChanged();
    }
  }
}

void RouteTracker::updateETA() {
  const qreal prevETA = m_targetETA;
  const qreal prevSETA = m_segmentETA;

  if (m_lineSpeed < .5) { // includes negative speeds
    m_targetETA = undefined;
    m_segmentETA = undefined;
    if (!std::isnan(prevETA)) {
      emit targetETAChanged();
    }
    if (!std::isnan(prevSETA)) {
      emit segmentETAChanged();
    }
    return;
  }

  m_targetETA = m_targetDTG / m_lineSpeed;
  m_segmentETA = m_segmentDTG / m_lineSpeed;

  // qDebug() << m_targetETA << m_segmentETA;

  if (std::isnan(prevETA)) {
    emit targetETAChanged();
  } else {
    if (static_cast<int>(m_targetETA / 60) != static_cast<int>(prevETA / 60)) {
      emit targetETAChanged();
    }
  }

  if (std::isnan(prevSETA)) {
    emit segmentETAChanged();
  } else {
    if (static_cast<int>(m_segmentETA / 60) != static_cast<int>(prevSETA / 60)) {
      emit segmentETAChanged();
    }
  }
}
