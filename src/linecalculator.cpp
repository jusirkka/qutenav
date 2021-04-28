/* -*- coding: utf-8-unix -*-
 *
 * File: src/linecalculator.cpp
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
#include "linecalculator.h"
#include <QOpenGLExtraFunctions>
#include <glm/glm.hpp>
#include "utils.h"

GL::LineCalculator* GL::LineCalculator::instance() {
  static LineCalculator* lc = new LineCalculator();
  return lc;
}

GL::LineCalculator::LineCalculator()
{}

GL::LineCalculator::~LineCalculator() {}

void GL::LineCalculator::calculate(VertexVector& transforms,
                                   GLfloat period,
                                   const QRectF& va,
                                   BufferData& vertices,
                                   BufferData& indices) {
  if (period < 1.e-10) {
    qWarning() << "GL::LineCalculator: Period is too small" << period;
    return;
  }
   // qDebug() << "period" << period;

  vertices.buffer.bind();
  auto vertexBufferIn = reinterpret_cast<const glm::vec2*>(
        vertices.buffer.mapRange(0, vertices.count * sizeof(glm::vec2),
                                 QOpenGLBuffer::RangeRead));

  indices.buffer.bind();
  auto indexBufferIn = reinterpret_cast<const GLuint*>(
        indices.buffer.mapRange(0, indices.count * sizeof(GLuint),
                                 QOpenGLBuffer::RangeRead));

  const int numOutIndices = indices.count - 1;
  for (int index = 0; index < numOutIndices; index++) {
    const uint v1 = vertices.offset + indexBufferIn[indices.offset + index];
    const uint v2 = vertices.offset + indexBufferIn[indices.offset + index + 1];
    const glm::vec2 p1 = vertexBufferIn[v1];
    const glm::vec2 p2 = vertexBufferIn[v2];

    if (outsideBox(p1, p2, va)) {
      continue;
    }

    const float r = glm::length(p2 - p1);
    if (r < 1.e-10) {
      continue;
    }
    float n = floor(r / period);
    // const float r0 = period * (n + .5);
    // if (r > r0 || n == 0) {
    //   n = n + 1;
    // }
    // const float s = r / (period * n);
    if (n == 0.) continue;

    // qDebug() << "number of symbols" << n;


    const glm::vec2 u = (p2 - p1) / r;
    const glm::vec2 dir = period * u;
    // const glm::vec2 dir = s * period * u;
    for (int k = 0; k < n; k++) {
      const glm::vec2 v = p1 + (1.f * k) * dir;
      // transforms << v.x << v.y << s * u.x << s * u.y;
      transforms << v.x << v.y << u.x << u.y;
    }
  }

  vertices.buffer.unmap();
  indices.buffer.unmap();
}



