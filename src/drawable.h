/* -*- coding: utf-8-unix -*-
 *
 * File: src/drawable.h
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


#include <QMatrix4x4>
#include <QOpenGLShaderProgram>


#include "camera.h"

class Drawable: public QObject {

  Q_OBJECT

public:

  Drawable(QObject* parent);

  virtual void paintGL(const Camera* camera) = 0;

  // OpenGL related stuff to be initialized after we have a GL context.
  virtual void initializeGL() = 0;

  // Called when chart manager has created new charts or the
  // charts have new content
  virtual void updateCharts(const Camera* cam, const QRectF& viewArea) = 0;

  virtual ~Drawable() = default;

};

using DrawableVector = QVector<Drawable*>;
