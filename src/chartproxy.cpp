/* -*- coding: utf-8-unix -*-
 *
 * File: chartproxy.cpp
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

#include "chartproxy.h"

GL::ChartProxy::ChartProxy(const VertexVector& vertices, const IndexVector& indices)
  : m_staticCoordBuffer(QOpenGLBuffer::VertexBuffer)
  , m_staticIndexBuffer(QOpenGLBuffer::IndexBuffer)
  , m_dynamicCoordBuffer(QOpenGLBuffer::VertexBuffer)
  , m_pivotBuffer(QOpenGLBuffer::VertexBuffer)
  , m_transformBuffer(QOpenGLBuffer::VertexBuffer)
  , m_textTransformBuffer(QOpenGLBuffer::VertexBuffer)
  , m_staticVertices(vertices)
  , m_staticIndices(indices)
{}


void GL::ChartProxy::create() {
  // fill in the buffers

  m_staticCoordBuffer.create();
  m_staticCoordBuffer.setUsagePattern(QOpenGLBuffer::StaticDraw);
  m_staticCoordBuffer.bind();
  const int coordBufSize = m_staticVertices.size() * sizeof(GLfloat);
  m_staticCoordBuffer.allocate(coordBufSize);
  m_staticCoordBuffer.write(0, m_staticVertices.constData(), coordBufSize);

  m_staticIndexBuffer.create();
  m_staticIndexBuffer.setUsagePattern(QOpenGLBuffer::StaticDraw);
  m_staticIndexBuffer.bind();
  const int indexBufSize = m_staticIndices.size() * sizeof(GLuint);
  m_staticIndexBuffer.allocate(indexBufSize);
  m_staticIndexBuffer.write(0, m_staticIndices.constData(), indexBufSize);

  m_dynamicCoordBuffer.create();
  m_dynamicCoordBuffer.setUsagePattern(QOpenGLBuffer::DynamicDraw);
  m_dynamicCoordBuffer.bind();
  // 1K line string vertices
  m_dynamicCoordBuffer.allocate(1000 * 2 * sizeof(GLfloat));

  m_pivotBuffer.create();
  m_pivotBuffer.setUsagePattern(QOpenGLBuffer::DynamicDraw);
  m_pivotBuffer.bind();
  // 5K raster symbol/pattern instances
  m_pivotBuffer.allocate(5000 * 2 * sizeof(GLfloat));

  m_transformBuffer.create();
  m_transformBuffer.setUsagePattern(QOpenGLBuffer::DynamicDraw);
  m_transformBuffer.bind();
  // 3K vector symbol/pattern instances
  m_transformBuffer.allocate(3000 * 4 * sizeof(GLfloat));

  m_textTransformBuffer.create();
  m_textTransformBuffer.setUsagePattern(QOpenGLBuffer::DynamicDraw);
  m_textTransformBuffer.bind();
  // 5K char instances
  m_textTransformBuffer.allocate(5000 * 10 * sizeof(GLfloat));
}

void GL::ChartProxy::update() {

  // update dynamic vertex buffer
  m_dynamicCoordBuffer.bind();
  GLsizei dataLen = sizeof(GLfloat) * m_dynamicVertices.size();
  if (dataLen > m_dynamicCoordBuffer.size()) {
    m_dynamicCoordBuffer.allocate(dataLen);
  }
  m_dynamicCoordBuffer.write(0, m_dynamicVertices.constData(), dataLen);
  m_dynamicVertices.clear();

  // update pivot buffer
  m_pivotBuffer.bind();
  dataLen = sizeof(GLfloat) * m_pivots.size();
  if (dataLen > m_pivotBuffer.size()) {
    m_pivotBuffer.allocate(dataLen);
  }
  m_pivotBuffer.write(0, m_pivots.constData(), dataLen);
  m_pivots.clear();

  // update transform buffer
  m_transformBuffer.bind();
  dataLen = sizeof(GLfloat) * m_transforms.size();
  if (dataLen > m_transformBuffer.size()) {
    m_transformBuffer.allocate(dataLen);
  }
  m_transformBuffer.write(0, m_transforms.constData(), dataLen);
  m_transforms.clear();

  // update text transform buffer
  m_textTransformBuffer.bind();
  dataLen = sizeof(GLfloat) * m_textTransforms.size();
  if (dataLen > m_textTransformBuffer.size()) {
    m_textTransformBuffer.allocate(dataLen);
  }
  m_textTransformBuffer.write(0, m_textTransforms.constData(), dataLen);
  m_textTransforms.clear();
}


GL::ChartProxyVector GL::ChartProxyQueue::consume() {
  m_mutex.lock();
  GL::ChartProxyVector out(m_proxies);
  m_proxies.clear();
  m_mutex.unlock();
  return out;
}

void GL::ChartProxyQueue::append(ChartProxy* p) {
  m_mutex.lock();
  m_proxies.append(p);
  m_mutex.unlock();
}
