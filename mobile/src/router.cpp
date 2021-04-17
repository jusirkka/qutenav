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

Router::Router(QQuickItem* parent)
  : QQuickItem(parent)
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
    const QSGGeometry::Point2D p = m_vertices[index];
    kid->setProperty("center", QPointF(p.x, p.y));
  }

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

  QSGGeometry::Point2D p;
  p.set(q.x(), q.y());
  m_vertices << p;

  if (wasEmpty) {
    emit emptyChanged();
  }

  update();

  return m_vertices.size() - 1;
}

void Router::move(int index, const QPointF& dp) {
  auto encdis = qobject_cast<const ChartDisplay*>(parentItem());
  if (encdis == nullptr) {
    qWarning() << "Expected ChartDisplay parent, cannot append";
    return;
  }

  QPointF q(m_vertices[index].x + dp.x(), m_vertices[index].y + dp.y());


  m_positions[index] = encdis->location(q);

  QSGGeometry::Point2D p2;
  p2.set(q.x(), q.y());
  m_vertices[index] = p2;

  for (QQuickItem* kid: childItems()) {
    auto k = QQmlProperty::read(kid, "index").toInt();
    if (k == index) {
      kid->setProperty("center", q);
      break;
    }
  }

  update();
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

  update();
}

bool Router::last(int index) const {
  return index == m_vertices.size() - 1;
}

QPointF Router::insert(int index) {

  auto encdis = qobject_cast<const ChartDisplay*>(parentItem());
  if (encdis == nullptr) {
    qWarning() << "Expected ChartDisplay parent, cannot append";
    return QPointF();
  }

  QPointF q(.5 * (m_vertices[index - 1].x + m_vertices[index].x),
            .5 * (m_vertices[index - 1].y + m_vertices[index].y));


  m_positions.insert(index, encdis->location(q));

  QSGGeometry::Point2D p;
  p.set(q.x(), q.y());

  m_vertices.insert(index, p);


  for (QQuickItem* kid: childItems()) {
    auto k = QQmlProperty::read(kid, "index").toInt();
    if (k >= index) {
      kid->setProperty("index", k + 1);
    }
  }

  update();

  return q;
}

QSGNode* Router::updatePaintNode(QSGNode* oldNode, UpdatePaintNodeData*) {
  QSGGeometryNode *node = nullptr;
  QSGGeometry *geometry = nullptr;

  if (!oldNode) {
    node = new QSGGeometryNode;
    geometry = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(),
                               m_vertices.size());
    geometry->setLineWidth(lineWidth);
    geometry->setDrawingMode(GL_LINE_STRIP);
    node->setGeometry(geometry);
    node->setFlag(QSGNode::OwnsGeometry);
    auto material = new QSGFlatColorMaterial;
    material->setColor(QColor("#214cad"));
    node->setMaterial(material);
    node->setFlag(QSGNode::OwnsMaterial);
  } else {
    node = static_cast<QSGGeometryNode*>(oldNode);
    geometry = node->geometry();
    if (geometry->vertexCount() != m_vertices.size()) {
      geometry->allocate(m_vertices.size());
    }
  }

  memcpy(geometry->vertexData(), m_vertices.constData(), m_vertices.size() * sizeof(QSGGeometry::Point2D));

  node->markDirty(QSGNode::DirtyGeometry);
  return node;
}

