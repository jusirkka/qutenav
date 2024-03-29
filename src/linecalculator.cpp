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
#include "geomutils.h"

GL::LineCalculator* GL::LineCalculator::instance() {
  static LineCalculator* lc = new LineCalculator();
  return lc;
}

GL::LineCalculator::LineCalculator()
{}

GL::LineCalculator::~LineCalculator() {}

void GL::LineCalculator::calculate(VertexVector& transforms,
                                   VertexVector& segments,
                                   GLfloat period,
                                   const QRectF& va,
                                   const VertexVector& vertices,
                                   const IndexVector& indices,
                                   const OffsetData& offs) {
  if (period < 1.e-10) {
    qWarning() << "GL::LineCalculator: Period is too small" << period;
    return;
  }
   // qDebug() << "period" << period;

  auto vertexBufferIn = reinterpret_cast<const glm::vec2*>(vertices.constData());

  auto indexBufferIn = indices.constData();

  const int numOutIndices = offs.count - 1;
  for (int index = 0; index < numOutIndices; index++) {
    const uint v1 = offs.vertexOffset + indexBufferIn[offs.indexOffset + index];
    const uint v2 = offs.vertexOffset + indexBufferIn[offs.indexOffset + index + 1];
    const glm::vec2 p1 = vertexBufferIn[v1];
    const glm::vec2 p2 = vertexBufferIn[v2];

    if (!crossesBox(p1, p2, va)) {
      continue;
    }

    const float r = glm::length(p2 - p1);
    if (r < 1.e-10) {
      continue;
    }
    auto n = static_cast<int>(floor(r / period));

    // qDebug() << "number of symbols" << n;

    const glm::vec2 u = (p2 - p1) / r;
    const glm::vec2 dir = period * u;
    for (int k = 0; k < n; k++) {
      const glm::vec2 v = p1 + (1.f * k) * dir;
      transforms << v.x << v.y << u.x << u.y;
    }
    if (r - n * period > .01) {// there is leftover
      const glm::vec2 v = p1 + (1.f * n) * dir;
      segments << v.x << v.y;
      segments << p2.x << p2.y;
    }
  }
}



