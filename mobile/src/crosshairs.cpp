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


QSGNode *CrossHairs::updatePaintNode(QSGNode* oldNode, UpdatePaintNodeData*)
{
  QSGGeometryNode *node = nullptr;
  QSGGeometry *geometry = nullptr;

  if (!oldNode) {
    node = new QSGGeometryNode;
    geometry = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), 8);
    geometry->setLineWidth(lineWidth);
    geometry->setDrawingMode(GL_LINES);
    node->setGeometry(geometry);
    node->setFlag(QSGNode::OwnsGeometry);
    auto material = new QSGFlatColorMaterial;
    material->setColor(QColor(255, 0, 0));
    node->setMaterial(material);
    node->setFlag(QSGNode::OwnsMaterial);
  } else {
    node = static_cast<QSGGeometryNode*>(oldNode);
    geometry = node->geometry();
    geometry->allocate(8);
  }
  const float x0 = static_cast<float>(m_peepHole.x());
  const float y0 = static_cast<float>(m_peepHole.y());
  QSGGeometry::Point2D* vertices = geometry->vertexDataAsPoint2D();

  vertices[0].set(x0 - lineWidth * 1.5, y0);
  vertices[1].set(0, y0);

  vertices[2].set(x0, y0 + lineWidth * 1.5);
  vertices[3].set(x0, window()->height());

  vertices[4].set(x0 + lineWidth * 1.5, y0);
  vertices[5].set(window()->width(), y0);

  vertices[6].set(x0, y0 - lineWidth * 1.5);
  vertices[7].set(x0, 0);

  node->markDirty(QSGNode::DirtyGeometry);
  return node;
}
