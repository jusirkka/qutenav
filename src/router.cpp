/* -*- coding: utf-8-unix -*-
 *
 * router.cpp
 *
 * Created: 17/04/2021 2021 by Jukka Sirkka
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
#include "router.h"
#include <QSGGeometryNode>
#include <QSGFlatColorMaterial>
#include <QDebug>
#include "chartdisplay.h"
#include <QQmlProperty>
#include "routedatabase.h"
#include "geomutils.h"

Router::Router(QQuickItem* parent)
  : QQuickItem(parent)
  , m_edited(false)
  , m_synced(false)
  , m_modified(false)
  , m_routeId(-1)
  , m_distance(0.)
{
  setFlag(ItemHasContents, true);
}

void Router::sync() {
  if (m_positions.isEmpty()) return;

  auto encdis = qobject_cast<const ChartDisplay*>(parentItem());
  if (encdis == nullptr) {
    qWarning() << "Expected ChartDisplay parent, cannot sync";
    return;
  }

  encdis->syncPositions(m_positions, m_vertices);

  for (QQuickItem* kid: childItems()) {
    auto index = QQmlProperty::read(kid, "index").toInt();
    kid->setProperty("center", m_vertices[index]);
  }

  m_synced = true;

  update();

}

int Router::append(const QPointF& q) {
  auto encdis = qobject_cast<const ChartDisplay*>(parentItem());
  if (encdis == nullptr) {
    qWarning() << "Expected ChartDisplay parent, cannot append";
    return -1;
  }

  const bool wasEmpty = m_vertices.isEmpty();

  m_positions << encdis->location(q);
  m_vertices << q;

  if (wasEmpty) {
    emit emptyChanged();
  }

  if (!m_edited) {
    m_edited = true;
    emit editedChanged();
  }

  m_modified = true;

  if (m_positions.size() > 1) {
    const auto last = m_positions.size() - 1;
    m_distance += (m_positions[last] - m_positions[last - 1]).meters();
    emit distanceChanged(m_distance);
  }

  update();

  return m_vertices.size() - 1;
}

void Router::move(int index, const QPointF& dp) {
  auto encdis = qobject_cast<const ChartDisplay*>(parentItem());
  if (encdis == nullptr) {
    qWarning() << "Expected ChartDisplay parent, cannot move";
    return;
  }

  const QPointF q = m_vertices[index] + dp;
  const auto pos = encdis->location(q);

  qreal oldDist = 0.;
  qreal newDist = 0.;
  if (index > 0) {
    oldDist += (m_positions[index] - m_positions[index - 1]).meters();
    newDist += (pos - m_positions[index - 1]).meters();
  }
  if (index < m_positions.size() - 1) {
    oldDist += (m_positions[index + 1] - m_positions[index]).meters();
    newDist += (m_positions[index + 1] - pos).meters();
  }

  m_positions[index] = pos;
  m_vertices[index] = q;

  for (QQuickItem* kid: childItems()) {
    auto k = QQmlProperty::read(kid, "index").toInt();
    if (k == index) {
      kid->setProperty("center", q);
      break;
    }
  }

  if (!m_edited) {
    m_edited = true;
    emit editedChanged();
  }

  m_synced = true;

  if (newDist != oldDist) {
    m_distance += newDist - oldDist;
    emit distanceChanged(m_distance);
  }

  update();
}

void Router::enableEditMode(bool enabled) {
  for (QQuickItem* kid: childItems()) {
    auto k = QQmlProperty::read(kid, "editMode").toBool();
    if (k != enabled) {
      kid->setProperty("editMode", enabled);
    }
  }
}

void Router::remove(int index) {

  m_positions.remove(index);
  m_vertices.remove(index);

  QQuickItem* target = nullptr;

  for (QQuickItem* kid: childItems()) {
    auto k = QQmlProperty::read(kid, "index").toInt();
    if (k == index) {
      target = kid;
    } else if (k > index) {
      kid->setProperty("index", k - 1);
    }
  }

  target->setParent(nullptr);
  target->setParentItem(nullptr);
  target->deleteLater();

  if (m_vertices.isEmpty()) {
    emit emptyChanged();
  }

  if (!m_edited) {
    m_edited = true;
    emit editedChanged();
  }

  m_modified = true;

  updateDistance();

  update();
}


void Router::clear() {

  const bool wasEmpty = m_vertices.isEmpty();

  m_positions.clear();
  m_vertices.clear();

  for (QQuickItem* kid: childItems()) {
    kid->setParent(nullptr);
    kid->setParentItem(nullptr);
    kid->deleteLater();
  }

  if (!wasEmpty) {
    emit emptyChanged();
  }

  if (m_edited) {
    m_edited = false;
    emit editedChanged();
  }

  m_routeId = -1;

  m_modified = true;
  m_synced = true;

  m_distance = 0.;

  update();
}

void Router::reverse() {

  std::reverse(m_positions.begin(), m_positions.end());
  std::reverse(m_vertices.begin(), m_vertices.end());

  for (QQuickItem* kid: childItems()) {
    auto index = QQmlProperty::read(kid, "index").toInt();
    kid->setProperty("center", m_vertices[index]);
  }

  if (!m_edited) {
    m_edited = true;
    emit editedChanged();
  }

  m_routeId = -1;

  m_modified = true;
  m_synced = true;

  update();
}

void Router::save() {

  RouteDatabase db("Router::save");

  if (m_positions.isEmpty()) {
    db.deleteRoute(m_routeId);
    m_routeId = -1;
    if (m_edited) {
      m_edited = false;
      emit editedChanged();
    }
    return;
  }

  try {
    if (m_routeId < 0) {
      m_routeId = db.createRoute(m_positions);
    } else {
      db.modifyRoute(m_routeId, m_positions);
    }
  } catch (DatabaseError& e) {
    qWarning() << e.msg();
  }

  if (m_edited) {
    m_edited = false;
    emit editedChanged();
  }
}

void Router::load(int rid) {

  auto encdis = qobject_cast<const ChartDisplay*>(parentItem());
  if (encdis == nullptr) {
    qWarning() << "Expected ChartDisplay parent, cannot load";
    return;
  }

  const bool wasEmpty = m_vertices.isEmpty();

  m_positions.clear();
  m_vertices.clear();

  for (QQuickItem* kid: childItems()) {
    kid->setParent(nullptr);
    kid->setParentItem(nullptr);
    kid->deleteLater();
  }

  RouteDatabase db("Router::load");
  auto r0 = db.prepare("select lng, lat from paths "
                       "where route_id = ? "
                       "order by id");
  r0.bindValue(0, rid);
  db.exec(r0);

  while (r0.next()) {
    const auto wp = WGS84Point::fromLL(r0.value(0).toReal(), r0.value(1).toReal());
    m_positions << wp;

    const auto q = encdis->position(wp);
    m_vertices << q;
  }

  m_routeId = m_vertices.isEmpty() ? -1 : rid;

  if (wasEmpty != m_vertices.isEmpty()) {
    emit emptyChanged();
  }

  m_modified = true;
  m_synced = true;

  updateDistance();

  // No need to test m_edited - archive page enabled only if there are no edits.

  update();
}

int Router::length() const {
  return m_vertices.size();
}


QPointF Router::vertex(int index) const {
  if (index < 0 || index >= m_vertices.size()) return QPointF();
  return m_vertices[index];
}

QPointF Router::insert(int index) {

  auto encdis = qobject_cast<const ChartDisplay*>(parentItem());
  if (encdis == nullptr) {
    qWarning() << "Expected ChartDisplay parent, cannot insert";
    return QPointF();
  }

  if (index < 1 || index >= m_vertices.size()) {
    qWarning() << "Invalid line (" <<  index - 1 << "," << index << ")";
    return QPointF();
  }

  const QPointF q = .5 * (m_vertices[index - 1] + m_vertices[index]);
  m_positions.insert(index, encdis->location(q));
  m_vertices.insert(index, q);

  for (QQuickItem* kid: childItems()) {
    auto k = QQmlProperty::read(kid, "index").toInt();
    if (k >= index) {
      kid->setProperty("index", k + 1);
    }
  }

  if (!m_edited) {
    m_edited = true;
    emit editedChanged();
  }

  m_modified = true;

  updateDistance();

  update();

  return q;
}

void Router::updateDistance() {
  qreal dist = 0.;
  for (int i = 1; i < m_positions.size(); ++i) {
    dist += (m_positions[i] - m_positions[i - 1]).meters();
  }
  if (m_distance != dist) {
    m_distance = dist;
    emit distanceChanged(m_distance);
  }
}

QString Router::name() const {
  if (m_routeId < 0) return "New Route";
  RouteDatabase db("Router::name");
  auto r0 = db.prepare("select name from routes where id = ?");
  r0.bindValue(0, m_routeId);
  db.exec(r0);
  r0.first();
  return r0.value(0).toString();
}

QSGNode* Router::updatePaintNode(QSGNode* oldNode, UpdatePaintNodeData*) {
  QSGGeometryNode *node = nullptr;
  QSGGeometry *geometry = nullptr;

  GL::VertexVector vertices;
  GL::IndexVector indices;

  // Triangulate
  if ((m_synced || m_modified) && m_vertices.size() >= 2) {
    thickerLines(m_vertices, false, lineWidth, vertices, indices);
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
    material->setColor(QColor("#214cad"));
    node->setMaterial(material);
    node->setFlag(QSGNode::OwnsMaterial);

  } else {
    node = static_cast<QSGGeometryNode*>(oldNode);
    geometry = node->geometry();
    if (geometry->vertexCount() != vertices.size() / 2 || geometry->indexCount() != indices.size()) {
      geometry->allocate(vertices.size() / 2, indices.size());
    }
  }

  if (m_synced || m_modified) {
    memcpy(geometry->vertexData(), vertices.constData(), vertices.size() * sizeof(GLfloat));
    geometry->markVertexDataDirty();
    node->markDirty(QSGNode::DirtyGeometry);
  }

  if (m_modified) {
    memcpy(geometry->indexData(), indices.constData(), indices.size() * sizeof(GLuint));
    geometry->markIndexDataDirty();
  }

  m_synced = false;
  m_modified = false;

  return node;
}

