/* -*- coding: utf-8-unix -*-
 *
 * routetracker.h
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
#pragma once

#include "types.h"
#include "LineString.h"

class RouteTracker: public QObject {

  Q_OBJECT

public:

  RouteTracker(QObject* parent = nullptr);

  int segmentEndPoint() const {return m_segmentEndPoint;}
  qreal segmentBearing() const {return m_segmentBearing;}
  qreal segmentETA() const {return m_segmentETA;} // seconds
  qreal segmentDTG() const {return m_segmentDTG;} // meters
  qreal targetETA() const {return m_targetETA;} // seconds
  qreal targetDTG() const {return m_targetDTG;} // meters

  void update(const WGS84Point& wp, qint64 msecs);
  void initialize(const WGS84PointVector& route);

signals:

  void segmentEndPointChanged();
  void segmentETAChanged();
  void segmentDTGChanged();
  void segmentBearingChanged();

  void targetETAChanged();
  void targetDTGChanged();

private:

  void updateSegment();
  void updateDTG();
  void updateETA();

  geos::geom::Coordinate fromWGS84Point(const WGS84Point& wp) const;

  static const inline qreal z0 = WGS84Point::semimajor_axis * 0.9996;
  static const inline qreal alpha = 1. - exp(- 1. / 60); // one minute line speed average

  int m_segmentEndPoint;

  qreal m_segmentBearing; // degrees
  qreal m_segmentETA;  // secs
  qreal m_segmentDTG; // meters

  qreal m_targetETA; // secs
  qreal m_targetDTG; // meters

  qint64 m_instant; // msecs; for computing line speed
  qreal m_position;
  qreal m_lineSpeed;

  geos::geom::LineString m_route;

  qreal m_c0; // euclidean projection factor
  WGS84Point m_ref;

};

