/* -*- coding: utf-8-unix -*-
 *
 * crosshairs.cpp
 *
 * Created: 07/02/2021 2021 by Jukka Sirkka
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
#include "crosshairs.h"
#include <QSGGeometryNode>
#include <QSGGeometry>
#include <QSGFlatColorMaterial>
#include <QQuickWindow>
#include <QDebug>

CrossHairs::CrossHairs(QQuickItem *parent)
  : QQuickItem(parent)
  , m_peepHole(100, 100)
  , m_indices {0, 1, 2, 1, 3, 2,
               4, 5, 6, 5, 7, 6,
               8, 9, 10, 9, 11, 10,
               12, 13, 14, 13, 15, 14}
{
  setFlag(ItemHasContents, true);
}

CrossHairs::~CrossHairs() {}

void CrossHairs::setPeepHole(const QPointF& p) {
  if (p != m_peepHole) {
    m_peepHole = p;
    emit peepHoleChanged(m_peepHole);
    update();
  }
}


QSGNode *CrossHairs::updatePaintNode(QSGNode* oldNode, UpdatePaintNodeData*) {
  QSGGeometryNode *node = nullptr;
  QSGGeometry *geometry = nullptr;

  if (!oldNode) {
    node = new QSGGeometryNode;
    geometry = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(),
                               16,
                               24,
                               GL_UNSIGNED_INT);
    geometry->setDrawingMode(GL_TRIANGLES);
    node->setGeometry(geometry);
    node->setFlag(QSGNode::OwnsGeometry);
    auto material = new QSGFlatColorMaterial;
    material->setColor(QColor(255, 0, 0));
    node->setMaterial(material);
    node->setFlag(QSGNode::OwnsMaterial);

    memcpy(geometry->indexData(), m_indices.constData(), m_indices.size() * sizeof(GLuint));

  } else {
    node = static_cast<QSGGeometryNode*>(oldNode);
    geometry = node->geometry();
  }

  const float x0 = static_cast<float>(m_peepHole.x());
  const float y0 = static_cast<float>(m_peepHole.y());
  const float d = .5 * KV::PeepHoleSize;
  QSGGeometry::Point2D* vertices = geometry->vertexDataAsPoint2D();

  vertices[0].set(0, y0 + lineWidth / 2);
  vertices[1].set(0, y0 - lineWidth / 2);
  vertices[2].set(x0 - d, y0 + lineWidth / 2);
  vertices[3].set(x0 - d, y0 - lineWidth / 2);

  vertices[4].set(x0 + d, y0 + lineWidth / 2);
  vertices[5].set(x0 + d, y0 - lineWidth / 2);
  vertices[6].set(window()->width(), y0 + lineWidth / 2);
  vertices[7].set(window()->width(), y0 - lineWidth / 2);

  vertices[8].set(x0 + lineWidth / 2, y0 + d);
  vertices[9].set(x0 - lineWidth / 2, y0 + d);
  vertices[10].set(x0 + lineWidth / 2, window()->height());
  vertices[11].set(x0 - lineWidth / 2, window()->height());

  vertices[12].set(x0 + lineWidth / 2, 0);
  vertices[13].set(x0 - lineWidth / 2, 0);
  vertices[14].set(x0 + lineWidth / 2, y0 - d);
  vertices[15].set(x0 - lineWidth / 2, y0 - d);

  node->markDirty(QSGNode::DirtyGeometry);
  return node;
}
