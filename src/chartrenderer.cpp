/* -*- coding: utf-8-unix -*-
 *
 * File: mobile/src/chartrenderer.cpp
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
#include "chartrenderer.h"
#include <QQuickWindow>
#include "camera.h"
#include "detailmode.h"
#include "chartdisplay.h"
#include <QOpenGLFramebufferObjectFormat>
#include "logging.h"
#include <QOpenGLDebugLogger>
#include "rastersymbolmanager.h"

ChartRenderer::ChartRenderer(QQuickWindow *window)
  : QQuickFramebufferObject::Renderer()
  , m_mode(DetailMode::RestoreState())
  , m_window(window)
  , m_logger(nullptr)
{
  m_vao.create();
  initializeGL();
}

void ChartRenderer::initializeGL() {

  m_vao.bind();

  for (Drawable* drawable: m_mode->drawables()) {
    drawable->initializeGL();
  }

  if (m_logger == nullptr) {
    m_logger = new QOpenGLDebugLogger;
    m_logger->initialize();
  }
  // I suspect Desktop/Mesa has a bug in textures / context sharing:
  // If initialized in Chartdisplay, raster symbols are not shown
  RasterSymbolManager::instance()->changeSymbolAtlas();
}


bool ChartRenderer::initializeChartMode() {
  DetailMode* mode = m_mode->smallerScaleMode();
  if (mode == nullptr) return false;
  qCDebug(CDPY) << "Initialize chart mode";
  delete m_mode;
  m_mode = mode;
  initializeGL();
  return true;
}

bool ChartRenderer::finalizeChartMode() {
  DetailMode* mode = m_mode->largerScaleMode();
  if (mode == nullptr) return false;
  qCDebug(CDPY) << "Finalize chart mode";
  delete m_mode;
  m_mode = mode;
  initializeGL();
  return true;
}


void ChartRenderer::render() {

  m_vao.bind();
  auto f = QOpenGLContext::currentContext()->functions();

  f->glClearStencil(0x00);
  f->glClearDepthf(1.);
  f->glClearColor(.4, .4, .4, 1.);
  f->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

  for (Drawable* drawable: m_mode->drawables()) {
    drawable->paintGL(m_mode->camera());
  }

  for (const QOpenGLDebugMessage& message: m_logger->loggedMessages()) {
    qCDebug(CDPY) << message;
  }

  m_window->resetOpenGLState();
}

QOpenGLFramebufferObject* ChartRenderer::createFramebufferObject(const QSize& size) {
  qCDebug(CDPY) << "ChartRenderer::createFramebufferObject";
  QOpenGLFramebufferObjectFormat fmt;
  fmt.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
  return new QOpenGLFramebufferObject(size, fmt);
}

ChartRenderer::~ChartRenderer() {
  qCDebug(CDPY) << "ChartRenderer::~ChartRenderer";
  m_mode->saveState();
  delete m_mode;
  delete m_logger;
}

void ChartRenderer::synchronize(QQuickFramebufferObject *parent) {

  m_vao.bind();

  auto item = qobject_cast<ChartDisplay*>(parent);

  m_mode->camera()->update(item->camera());

  if (item->consume(ChartDisplay::ChartsUpdated)) {
    for (Drawable* d: m_mode->drawables()) {
      d->updateCharts(m_mode->camera(), item->viewArea());
    }
  }

  if (item->consume(ChartDisplay::EnteringChartMode)) {
    if (initializeChartMode()) {
      item->setCamera(m_mode->cloneCamera());
    }
  }

  if (item->consume(ChartDisplay::LeavingChartMode)) {
    item->checkChartSet();
    if (finalizeChartMode()) {
      item->setCamera(m_mode->cloneCamera());
    }
  }

  if (item->consume(ChartDisplay::ChartSetChanged)) {
    initializeGL();
  }

  if (item->consume(ChartDisplay::ColorTableChanged)) {
    // I suspect Desktop/Mesa has a bug in textures / context sharing:
    // If initialized in Chartdisplay, raster symbols are not shown
    RasterSymbolManager::instance()->changeSymbolAtlas();
  }

  m_window->resetOpenGLState();

  for (const QOpenGLDebugMessage& message: m_logger->loggedMessages()) {
    qCDebug(CDPY) << message;
  }

}

