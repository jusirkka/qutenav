/* -*- coding: utf-8-unix -*-
 *
 * File: src/linecalculator.h
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

#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include "types.h"

namespace GL {

class LineCalculator {
public:

  static LineCalculator* instance();

  struct BufferData {
    QOpenGLBuffer buffer;
    GLsizei offset;
    GLsizei count;
  };

  void calculate(VertexVector& transforms, GLfloat period, const QRectF& va,
                 BufferData& vertices, BufferData& indices);

  ~LineCalculator();

private:

  const int compVertexBufferInBinding = 0;
  const int compIndexBufferInBinding = 1;
  const int compVertexBufferOutBinding = 2;

  LineCalculator();

};

}
