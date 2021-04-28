/* -*- coding: utf-8-unix -*-
 *
 * File: src/globe.h
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

#include "drawable.h"
#include <QOpenGLBuffer>
#include <QColor>

class QDir;
class QOpenGLTexture;

class Globe: public Drawable
{

  Q_OBJECT

public:

  Globe(QObject* parent);

  void paintGL(const Camera* cam) override;
  void initializeGL() override;
  void updateCharts(const Camera* cam, const QRectF& viewArea) override;

  ~Globe();

private:

  void loadImages(const QDir& imageDir);
  void loadImage(const QDir& imageDir);

  QOpenGLBuffer m_coordBuffer;
  QOpenGLBuffer m_indexBuffer;
  QOpenGLShaderProgram* m_program;
  QOpenGLTexture* m_globeTexture;

  GLsizei m_indexCount;

  struct _locations {
    int globe;
    int m_pv;
    int lp;
  } m_locations;

};

