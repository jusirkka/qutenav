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

Tracker::Tracker(QQuickItem* parent)
  : QQuickItem(parent)
  , m_status(Inactive)
  , m_lastIndex(-1)
  , m_duration(0)
  , m_speed(0)
  , m_distance(0)
{
  setFlag(ItemHasContents, true);
}

void Tracker::start() {
  if (m_status == Tracking) return;
  m_status = Tracking;
  emit statusChanged();
}

void Tracker::append(qreal lng, qreal lat) {
  if (m_status != Tracking) return;

  auto encdis = qobject_cast<const ChartDisplay*>(parentItem());
  if (encdis == nullptr) {
    qWarning() << "Expected ChartDisplay parent, cannot append";
    return;
  }

  const auto wp = WGS84Point::fromLL(lng, lat);
  const auto now = QDateTime::currentMSecsSinceEpoch();

  if (m_lastIndex < 0) {
    m_lastIndex = m_positions.size();
  } else {
    const auto dist = (wp - m_positions[m_lastIndex]).meters();
    if (dist < mindist) {
      // qDebug() << "Distance to previous point =" << dist << ", skipping";
      return;
    }
    const qreal delta = (now - m_instants[m_lastIndex]) * .001;
    // speed
    const auto speed = dist / delta;
    // qDebug() << "Speed is " << speed << delta;
    if (speed > maxSpeed) {
      qDebug() << "Speed is " << speed * toKnots << "knots, skipping";
      return;
    }
    const auto prevSpeed = m_speed;
    m_speed = speed;
    if (std::abs(m_speed - prevSpeed) / m_speed > eps) {
      emit speedChanged();
    }
    // duration
    const auto prevDuration = static_cast<int>(m_duration / 60);
    m_duration += delta;
    if (static_cast<int>(m_duration / 60) != prevDuration) {
      emit durationChanged();
    }
    // distance
    const auto prevDistance = static_cast<int>(m_distance / 185.2); // decimal nautical miles
    m_distance += dist;
    if (static_cast<int>(m_distance / 185.2) != prevDistance) {
      emit distanceChanged();
    }
    m_indices << m_lastIndex;
    m_lastIndex = m_positions.size();
    m_indices << m_lastIndex;
  }

  m_positions << wp;
  m_instants << now;
  m_vertices << fromPoint(encdis->position(wp));

  update();
}

void Tracker::sync() {
  if (m_positions.isEmpty()) return;

  auto encdis = qobject_cast<const ChartDisplay*>(parentItem());
  if (encdis == nullptr) {
    qWarning() << "Expected ChartDisplay parent, cannot sync";
    return;
  }

  encdis->syncPositions(m_positions, m_vertices);

  update();
}

void Tracker::pause() {
  if (m_status == Paused) return;
  m_lastIndex = -1;
  m_speed = 0;
  emit speedChanged();
  m_status = Paused;
  emit statusChanged();
}

void Tracker::save() {
  try {
    TrackDatabase db("Tracker::save");
    db.createTrack(m_instants, m_positions, m_indices);
    remove();
  } catch (DatabaseError& e){
    qWarning() << e.msg();
  }

}

void Tracker::remove() {
  m_positions.clear();
  m_vertices.clear();
  m_indices.clear();
  m_instants.clear();
  m_lastIndex = -1;
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
}

void Tracker::display() {

  m_positions.clear();
  m_vertices.clear();
  m_indices.clear();
  m_instants.clear();

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
        m_lastIndex = -1;
      }
      prev = string_id;

      if (m_lastIndex < 0) {
        m_lastIndex = m_positions.size();
      } else {
        m_indices << m_lastIndex;
        m_lastIndex = m_positions.size();
        m_indices << m_lastIndex;
      }

      m_positions << WGS84Point::fromLL(r0.value(2).toReal(), r0.value(3).toReal());
      m_instants << r0.value(1).toLongLong();

    }

    if (!m_positions.isEmpty()) {
      m_vertices.resize(m_positions.size());
      sync();
      if (m_status != Displaying) {
        m_status = Displaying;
        emit statusChanged();
      }
    } else {
      if (m_status != Inactive) {
        m_status = Inactive;
        emit statusChanged();
      }
      update();
    }


  } catch (DatabaseError& e) {
    qWarning() << e.msg();
  }
}

QSGNode *Tracker::updatePaintNode(QSGNode* oldNode, UpdatePaintNodeData*) {
  QSGGeometryNode *node = nullptr;
  QSGGeometry *geometry = nullptr;

  if (!oldNode) {
    node = new QSGGeometryNode;
    geometry = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(),
                               m_vertices.size(),
                               m_indices.size(),
                               GL_UNSIGNED_INT);
    geometry->setLineWidth(lineWidth);
    geometry->setDrawingMode(GL_LINES);
    node->setGeometry(geometry);
    node->setFlag(QSGNode::OwnsGeometry);
    auto material = new QSGFlatColorMaterial;
    material->setColor(QColor(255, 255, 0));
    node->setMaterial(material);
    node->setFlag(QSGNode::OwnsMaterial);
  } else {
    node = static_cast<QSGGeometryNode*>(oldNode);
    geometry = node->geometry();
    if (geometry->vertexCount() != m_vertices.size() || geometry->indexCount() != m_indices.size()) {
      geometry->allocate(m_vertices.size(), m_indices.size());
    }
  }

  memcpy(geometry->vertexData(), m_vertices.constData(), m_vertices.size() * sizeof(QSGGeometry::Point2D));
  memcpy(geometry->indexData(), m_indices.constData(), m_indices.size() * sizeof(GLuint));

  node->markDirty(QSGNode::DirtyGeometry);
  return node;
}

