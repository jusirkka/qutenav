/* -*- coding: utf-8-unix -*-
 *
 * File: mobile/src/chartrenderer.h
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

#include <QtQuick/QQuickFramebufferObject>
#include <QOpenGLVertexArrayObject>

class QQuickWindow;
class DetailMode;
class QOpenGLDebugLogger;

class ChartRenderer: public QQuickFramebufferObject::Renderer {

public:

  ChartRenderer(QQuickWindow *window);

  ~ChartRenderer();

protected:

  void render() override;
  void synchronize(QQuickFramebufferObject* parent) override;
  QOpenGLFramebufferObject* createFramebufferObject(const QSize &size) override;


private:

  bool initializeChartMode();
  bool finalizeChartMode();
  void initializeGL();

  DetailMode* m_mode;
  QQuickWindow* m_window;
  QOpenGLVertexArrayObject m_vao;
  QOpenGLDebugLogger* m_logger;

};


