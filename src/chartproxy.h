/* -*- coding: utf-8-unix -*-
 *
 * File: chartproxy.h
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
#include <QMutex>
#include <QVector>
#include <QOpenGLBuffer>
#include "types.h"

class S57Chart;

namespace GL {


class ChartProxy {

  friend class ::S57Chart;

public:

  ChartProxy(const VertexVector& vertices, const IndexVector& indices);
  ChartProxy() = default;
  ~ChartProxy() = default;

  void create();
  void update();

  void initializeGL();
  void finalizeGL();

private:

  QOpenGLBuffer m_staticCoordBuffer;
  QOpenGLBuffer m_staticIndexBuffer;
  QOpenGLBuffer m_dynamicCoordBuffer;
  QOpenGLBuffer m_pivotBuffer;
  QOpenGLBuffer m_transformBuffer;
  QOpenGLBuffer m_textTransformBuffer;

  const VertexVector m_staticVertices;
  const IndexVector m_staticIndices;

  // tmp data
  VertexVector m_dynamicVertices;
  VertexVector m_pivots;
  VertexVector m_transforms;
  VertexVector m_textTransforms;

};

using ChartProxyVector = QVector<ChartProxy*>;

class ChartProxyQueue {
public:

  ChartProxyQueue() = default;
  ~ChartProxyQueue() = default;

  ChartProxyVector consume();
  void append(ChartProxy* p);

private:

  ChartProxyVector m_proxies;
  QMutex m_mutex;

};

}

Q_DECLARE_METATYPE(GL::ChartProxy*)
