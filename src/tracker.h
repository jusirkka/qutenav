/* -*- coding: utf-8-unix -*-
 *
 * tracker.h
 *
 * Created: 09/04/2021 2021 by Jukka Sirkka
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

#include <QQuickItem>
#include <QSGGeometry>
#include "event.h"
#include "routetracker.h"
#include <QGeoCoordinate>

class Tracker: public QQuickItem {

  Q_OBJECT

public:

  Tracker(QQuickItem* parent = nullptr);

  Q_PROPERTY(Status status
             READ status
             NOTIFY statusChanged)

  enum Status {Inactive, Tracking, Paused, Displaying};
  Q_ENUM(Status)

  Status status() const {return m_status;}

  Q_PROPERTY(qreal duration
             READ duration
             NOTIFY durationChanged)

  qreal duration() const {return m_duration;} // seconds

  Q_PROPERTY(qreal distance
             READ distance
             NOTIFY distanceChanged)

  qreal distance() const {return m_distance;} // meters

  Q_PROPERTY(qreal speed
             READ speed
             NOTIFY speedChanged)

  qreal speed() const {return m_speed;} // meters / second

  Q_PROPERTY(qreal bearing
             READ bearing
             NOTIFY bearingChanged)

  qreal bearing() const {return m_bearing;}

  Q_PROPERTY(int segmentEndPoint
             READ segmentEndPoint
             NOTIFY segmentEndPointChanged)

  int segmentEndPoint() const {return m_router.segmentEndPoint();}

  Q_PROPERTY(qreal segmentETA
             READ segmentETA
             NOTIFY segmentETAChanged)

  qreal segmentETA() const {return m_router.segmentETA();}

  Q_PROPERTY(qreal segmentDTG
             READ segmentDTG
             NOTIFY segmentDTGChanged)

  qreal segmentDTG() const {return m_router.segmentDTG();}

  Q_PROPERTY(qreal segmentBearing
             READ segmentBearing
             NOTIFY segmentBearingChanged)

  qreal segmentBearing() const {return m_router.segmentBearing();}

  Q_PROPERTY(qreal targetETA
             READ targetETA
             NOTIFY targetETAChanged)

  qreal targetETA() const {return m_router.targetETA();}

  Q_PROPERTY(qreal targetDTG
             READ targetDTG
             NOTIFY targetDTGChanged)

  qreal targetDTG() const {return m_router.targetDTG();}


  Q_INVOKABLE void append(const QGeoCoordinate& q);
  Q_INVOKABLE void sync();

  Q_INVOKABLE void start();
  Q_INVOKABLE void reset();
  Q_INVOKABLE void pause();
  Q_INVOKABLE void save();
  Q_INVOKABLE void remove();
  Q_INVOKABLE void display();

  QSGNode *updatePaintNode(QSGNode* node, UpdatePaintNodeData*) override;


signals:

  void statusChanged();

  void durationChanged();
  void speedChanged();
  void distanceChanged();
  void bearingChanged();

  void segmentEndPointChanged();
  void segmentBearingChanged();
  void segmentETAChanged();
  void segmentDTGChanged();
  void targetETAChanged();
  void targetDTGChanged();

public slots:

private:

  void updateDistance(qreal v);
  void updateBearing(qreal v);
  void updateSpeed(qreal v);
  void updateDuration(qreal v);

  QSGGeometry::Point2D fromPoint(const QPointF& p) {
    QSGGeometry::Point2D v;
    v.set(p.x(), p.y());
    return v;
  }

  static const int lineWidth = 8;
  static const inline qreal mindist = 10.; // meters
  static const inline qreal maxSpeed = 30; // meters / sec
  static const inline qreal eps = .05;

  using PointVector = QVector<QPointF>;
  using TrackVector = QVector<PointVector>;

  TrackVector m_trackpoints;
  KV::EventStringVector m_events;

  bool m_synced;
  bool m_appended;

  Status m_status;

  qreal m_duration; // secs
  qreal m_speed; // meters / sec
  qreal m_lastSpeed; // for emit control
  qreal m_distance; // meters
  qreal m_bearing; // degrees

  RouteTracker m_router;
};

inline void Tracker::updateDistance(qreal dist) {
  const auto prevDistance = static_cast<int>(m_distance / 100); // 100m resolution
  m_distance += dist;
  if (static_cast<int>(m_distance / 100) != prevDistance) {
    emit distanceChanged();
  }
}

inline void Tracker::updateBearing(qreal deg) {
  while (deg < 0.) deg += 360.;
  // bearing
  const auto prevBearing = static_cast<int>(m_bearing);
  m_bearing = deg;
  if (static_cast<int>(m_bearing) != prevBearing) {
    emit bearingChanged();
  }
}

inline void Tracker::updateSpeed(qreal speed) {
  m_speed = speed;
  if (std::abs(m_speed - m_lastSpeed) / m_speed > eps) {
    m_lastSpeed = m_speed;
    emit speedChanged();
  }
}

inline void Tracker::updateDuration(qreal delta) {
  // duration
  const auto prevDuration = static_cast<int>(m_duration / 60); // one minute resolution
  m_duration += delta;
  if (static_cast<int>(m_duration / 60) != prevDuration) {
    emit durationChanged();
  }
}
