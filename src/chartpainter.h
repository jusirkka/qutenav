/* -*- coding: utf-8-unix -*-
 *
 * File: src/chartpainter.h
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
#include <s57object.h>

class S57Chart;
class GeoProjection;
class ChartManager;
class QOpenGLFramebufferObject;
class QOpenGLDebugLogger;

namespace GL {
class AreaShader;
class LineElemShader;
class LineArrayShader;
class SegmentArrayShader;
class TextShader;
class RasterSymbolShader;
class VectorSymbolShader;
class TextureShader;
}

class ChartPainter: public Drawable {
  Q_OBJECT

public:
  ChartPainter(QObject* parent);

  void paintGL(const Camera* cam) override;
  void initializeGL() override;
  void updateCharts(const Camera* cam, const QRectF& viewArea) override;

  ~ChartPainter();

private:

  Camera* createBufferCamera(const Camera* cam, const QSizeF& vp) const;

  ChartManager* m_manager;
  GL::TextureShader* m_textureShader;

  bool m_initialized;
  QRectF m_viewArea;
  WGS84Point m_ref;
  QSizeF m_bufSize;
  QOpenGLFramebufferObject* m_fbo;

  QOpenGLBuffer m_coordBuffer;
  QOpenGLBuffer m_indexBuffer;

  QOpenGLDebugLogger* m_logger;
};

