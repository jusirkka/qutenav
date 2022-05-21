/* -*- coding: utf-8-unix -*-
 *
 * chartindicator.cpp
 *
 * Copyright (C) 2022 Jukka Sirkka
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
#include "chartindicator.h"
#include <QSGGeometryNode>
#include <QSGFlatColorMaterial>
#include <QDebug>
#include "chartdisplay.h"
#include "chartmanager.h"
#include "geomutils.h"

ChartIndicator::ChartIndicator(QQuickItem* parent)
  : QQuickItem(parent)
  , m_synced(false)
  , m_reset(false)
{
  setFlag(ItemHasContents, true);
  connect(ChartManager::instance(), &ChartManager::chartIndicatorsChanged, this, &ChartIndicator::reset);
}

void ChartIndicator::sync() {
  if (m_positions.isEmpty()) return;

  auto encdis = qobject_cast<const ChartDisplay*>(parentItem());
  if (encdis == nullptr) {
    qWarning() << "Expected ChartDisplay parent, cannot sync";
    return;
  }

  for (int i = 0; i < m_positions.size(); ++i) {
    encdis->syncPositions(m_positions[i], m_vertices[i]);
  }

  m_synced = true;

  update();

}

void ChartIndicator::reset(const WGS84Polygon& indicators) {
  m_positions = indicators;

  auto encdis = qobject_cast<const ChartDisplay*>(parentItem());
  if (encdis == nullptr) {
    qWarning() << "Expected ChartDisplay parent, cannot sync";
    return;
  }
  m_vertices.resize(m_positions.size());
  for (int i = 0; i < m_positions.size(); ++i) {
    m_vertices[i].resize(m_positions[i].size());
    encdis->syncPositions(m_positions[i], m_vertices[i]);
  }

  m_reset = true;

  update();

}


QSGNode* ChartIndicator::updatePaintNode(QSGNode* oldNode, UpdatePaintNodeData*) {
  QSGGeometryNode *node = nullptr;
  QSGGeometry *geometry = nullptr;

  GL::VertexVector vertices;
  GL::IndexVector indices;

  // Triangulate
  if (m_synced || m_reset) {
    for (const PointVector& ps: m_vertices) {
      thickerLines(ps, true, lineWidth, vertices, indices);
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
    material->setColor(QColor("#2d962d"));
    node->setMaterial(material);
    node->setFlag(QSGNode::OwnsMaterial);

  } else if (m_synced || m_reset) {
    node = static_cast<QSGGeometryNode*>(oldNode);
    geometry = node->geometry();
    if (geometry->vertexCount() != vertices.size() / 2 || geometry->indexCount() != indices.size()) {
      geometry->allocate(vertices.size() / 2, indices.size());
    }
  }

  if (m_synced || m_reset) {
    memcpy(geometry->vertexData(), vertices.constData(), vertices.size() * sizeof(GLfloat));
    geometry->markVertexDataDirty();
    node->markDirty(QSGNode::DirtyGeometry);
  }

  if (m_reset) {
    memcpy(geometry->indexData(), indices.constData(), indices.size() * sizeof(GLuint));
    geometry->markIndexDataDirty();
  }

  m_synced = false;
  m_reset = false;

  return node;
}

