/* -*- coding: utf-8-unix -*-
 *
 * chartindicator.h
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
#pragma once


#include <QQuickItem>
#include <QSGGeometry>
#include "types.h"


class ChartIndicator: public QQuickItem {

  Q_OBJECT

public:

  ChartIndicator(QQuickItem* parent = nullptr);


  Q_INVOKABLE void sync();

  QSGNode *updatePaintNode(QSGNode* node, UpdatePaintNodeData*) override;


private slots:

  void reset(const WGS84Polygon& indicators);

private:

  static const int lineWidth = 4;

  using PointVector = QVector<QPointF>;
  using PPolygon = QVector<PointVector>;

  PPolygon m_vertices;
  WGS84Polygon m_positions;

  bool m_synced;
  bool m_reset;
};

