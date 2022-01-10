/* -*- coding: utf-8-unix -*-
 *
 * tracker.cpp
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

#include "tracker.h"
#include <QSGGeometryNode>
#include <QSGFlatColorMaterial>
#include <QDebug>
#include "chartdisplay.h"
#include <QDateTime>
#include "trackmodel.h"
#include "router.h"
#include "geomutils.h"

Tracker::Tracker(QQuickItem* parent)
  : QQuickItem(parent)
  , m_trackpoints({PointVector()})
  , m_events({KV::EventString()})
  , m_synced(false)
  , m_appended(false)
  , m_status(Inactive)
  , m_duration(0)
  , m_speed(0)
  , m_lastSpeed(-1.e20)
  , m_distance(0)
  , m_bearing(0.)
  , m_router()
{
  setFlag(ItemHasContents, true);

  connect(&m_router, &RouteTracker::segmentEndPointChanged,
          this, &Tracker::segmentEndPointChanged);
  connect(&m_router, &RouteTracker::segmentBearingChanged,
          this, &Tracker::segmentBearingChanged);
  connect(&m_router, &RouteTracker::segmentETAChanged,
          this, &Tracker::segmentETAChanged);
  connect(&m_router, &RouteTracker::segmentDTGChanged,
          this, &Tracker::segmentDTGChanged);
  connect(&m_router, &RouteTracker::targetETAChanged,
          this, &Tracker::targetETAChanged);
  connect(&m_router, &RouteTracker::targetDTGChanged,
          this, &Tracker::targetDTGChanged);
}

void Tracker::start() {

  auto encdis = qobject_cast<const ChartDisplay*>(parentItem());
  if (encdis == nullptr) {
    qWarning() << "Expected ChartDisplay parent, cannot start";
    return;
  }

  auto items = encdis->childItems();
  auto it = std::find_if(items.cbegin(), items.cend(), [] (const QQuickItem* item) {
    return dynamic_cast<const Router*>(item) != nullptr;
  });
  if (it == items.cend()) {
    qWarning() << "Expected ChartDisplay to contain a Router, cannot start";
    return;
  }

  if (m_status == Displaying) {
    remove();
  }

  auto rt = dynamic_cast<const Router*>(*it);
  m_router.initialize(rt->waypoints());

  if (m_status != Tracking) {
    m_status = Tracking;
    emit statusChanged();
  }
}

void Tracker::reset() {
  if (m_status != Tracking) return;
  start();
}

void Tracker::append(const QGeoCoordinate& q) {
  if (m_status != Tracking) return;

  auto encdis = qobject_cast<const ChartDisplay*>(parentItem());
  if (encdis == nullptr) {
    qWarning() << "Expected ChartDisplay parent, cannot append";
    return;
  }

  const auto wp = WGS84Point::fromLL(q.longitude(), q.latitude());
  const auto now = QDateTime::currentMSecsSinceEpoch();

  if (!m_events.last().isEmpty()) {
    const WGS84Bearing b = wp - m_events.last().last().position;
    const auto dist = b.meters();
    if (dist < mindist) {
      // qDebug() << "Distance to previous point =" << dist << ", skipping";
      return;
    }

    const qreal delta = (now - m_events.last().last().instant) * .001;

    const auto speed = dist / delta;
    if (speed > maxSpeed) {
      // qDebug() << "Speed is " << speed * toKnots << "knots, skipping";
      return;
    }

    updateDistance(dist);
    updateBearing(b.degrees());
    updateSpeed(speed);
    updateDuration(delta);
  }

  m_events.last() << KV::Event(now, wp);
  m_trackpoints.last() << encdis->position(wp);

  m_appended = true;

  m_router.update(wp, now);

  update();
}


void Tracker::sync() {

  auto encdis = qobject_cast<const ChartDisplay*>(parentItem());
  if (encdis == nullptr) {
    qWarning() << "Expected ChartDisplay parent, cannot sync";
    return;
  }

  for (int i = 0; i < m_events.size(); i++) {
    encdis->syncPositions(m_events[i], m_trackpoints[i]);
  }

  m_synced = true;
  update();
}

void Tracker::pause() {
  if (m_status == Paused) return;
  if (!m_events.last().isEmpty()) {
    m_events << KV::EventString();
    m_trackpoints << PointVector();
  }
  m_speed = 0;
  emit speedChanged();
  m_status = Paused;
  emit statusChanged();
}

void Tracker::save() {
  try {
    TrackDatabase db("Tracker::save");
    db.createTrack(m_events);
    remove();
  } catch (DatabaseError& e){
    qWarning() << e.msg();
  }

}

void Tracker::remove() {
  m_events.clear();
  m_trackpoints.clear();
  m_events << KV::EventString();
  m_trackpoints << PointVector();
  m_duration = 0;
  emit durationChanged();
  m_speed = 0;
  emit speedChanged();
  m_distance = 0;
  emit distanceChanged();
  if (m_status != Inactive) {
    m_status = Inactive;
    emit statusChanged();
  }

  m_appended = true;
  m_synced = true;
}

void Tracker::display() {

  auto encdis = qobject_cast<const ChartDisplay*>(parentItem());
  if (encdis == nullptr) {
    qWarning() << "Expected ChartDisplay parent, cannot display tracks";
    return;
  }

  m_events.clear();
  m_trackpoints.clear();

  try {

    TrackDatabase db("Tracker::display");

    auto r0 = db.exec("select e.string_id, e.time, e.lng, e.lat from events e "
                      "join strings s on e.string_id = s.id "
                      "join tracks t on s.track_id = t.id "
                      "where t.enabled != 0 "
                      "order by e.string_id, e.id");
    int prev = - 1;
    while (r0.next()) {

      const auto string_id = r0.value(0).toInt();
      if (string_id != prev) {
        m_events << KV::EventString();
        m_trackpoints << PointVector();
      }
      prev = string_id;

      auto wp = WGS84Point::fromLL(r0.value(2).toReal(), r0.value(3).toReal());
      m_events.last() << KV::Event(r0.value(1).toLongLong(), wp);
      m_trackpoints.last() << encdis->position(wp);
    }

    if (!m_events.isEmpty()) {
      if (m_status != Displaying) {
        m_status = Displaying;
        emit statusChanged();
      }
    } else {
      m_events << KV::EventString();
      m_trackpoints << PointVector();
      if (m_status != Inactive) {
        m_status = Inactive;
        emit statusChanged();
      }
    }


  } catch (DatabaseError& e) {
    qWarning() << e.msg();
  }

  m_appended = true;
  m_synced = true;
}

QSGNode *Tracker::updatePaintNode(QSGNode* oldNode, UpdatePaintNodeData*) {
  QSGGeometryNode *node = nullptr;
  QSGGeometry *geometry = nullptr;

  GL::VertexVector vertices;
  GL::IndexVector indices;

  // Triangulate
  if (m_synced || m_appended) {
    for (const PointVector& ps: m_trackpoints) {
      if (ps.size() < 2) continue;
      thickerLines(ps, false, lineWidth, vertices, indices);
    }
  }

  if (!oldNode) {
    node = new QSGGeometryNode;
    geometry = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(),
                               vertices.size() / 2,
                               indices.size(),
                               GL_UNSIGNED_INT);
    geometry->setDrawingMode(GL_TRIANGLES);
    geometry->setVertexDataPattern(QSGGeometry::DynamicPattern);
    geometry->setIndexDataPattern(QSGGeometry::DynamicPattern);
    node->setGeometry(geometry);
    node->setFlag(QSGNode::OwnsGeometry);

    auto material = new QSGFlatColorMaterial;
    material->setColor(QColor(255, 255, 0));
    node->setMaterial(material);
    node->setFlag(QSGNode::OwnsMaterial);

  } else {
    node = static_cast<QSGGeometryNode*>(oldNode);
    geometry = node->geometry();
    if (geometry->vertexCount() != vertices.size() / 2 || geometry->indexCount() != indices.size()) {
      geometry->allocate(vertices.size() / 2, indices.size());
    }
  }

  if (m_synced || m_appended) {
    memcpy(geometry->vertexData(), vertices.constData(), vertices.size() * sizeof(GLfloat));
    geometry->markVertexDataDirty();
    node->markDirty(QSGNode::DirtyGeometry);
  }

  if (m_appended) {
    memcpy(geometry->indexData(), indices.constData(), indices.size() * sizeof(GLuint));
    geometry->markIndexDataDirty();
  }

  m_synced = false;
  m_appended = false;

  return node;
}

