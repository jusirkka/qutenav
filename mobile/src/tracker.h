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
#include "types.h"
#include "trackdatabase.h"


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

  Q_PROPERTY(quint32 duration
             READ duration
             NOTIFY durationChanged)

  quint32 duration() const {return m_duration / 60;} // minutes

  Q_PROPERTY(qreal distance
             READ distance
             NOTIFY distanceChanged)

  qreal distance() const {return m_distance / 1852.;} // nautical miles

  Q_PROPERTY(qreal speed
             READ speed
             NOTIFY speedChanged)

  qreal speed() const {return m_speed * toKnots;} // knots


  Q_INVOKABLE void append(qreal lng, qreal lat);
  Q_INVOKABLE void sync();

  Q_INVOKABLE void start();
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


public slots:

private:

  QSGGeometry::Point2D fromPoint(const QPointF& p) {
    QSGGeometry::Point2D v;
    v.set(p.x(), p.y());
    return v;
  }

  static const int lineWidth = 8;
  static const inline qreal mindist = 10.; // meters
  static const inline qreal toKnots = 3600. / 1852.;
  static const inline qreal maxSpeed = 15. / toKnots; // meters / sec, = 15 knots
  static const inline qreal eps = .05;

  using PointVector = QVector<QSGGeometry::Point2D>;

  PointVector m_vertices;
  GL::IndexVector m_indices;

  WGS84PointVector m_positions;
  InstantVector m_instants;

  Status m_status;
  int m_lastIndex;

  quint32 m_duration; // secs
  qreal m_speed; // meters / sec
  qreal m_distance; // meters
};

