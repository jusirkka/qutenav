/* -*- coding: utf-8-unix -*-
 *
 * File: src/chartmode.cpp
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
#include "chartmode.h"
#include "orthocam.h"
#include "outlinemode.h"
#include "chartpainter.h"

ChartMode::ChartMode(float wmm, float hmm, GeoProjection* p)
  : DetailMode()
{
  m_camera = new OrthoCam(wmm, hmm, p);
  m_drawables << new ChartPainter(this);
}

ChartMode::~ChartMode() {
  delete m_camera;
  qDeleteAll(m_drawables);
}

DetailMode* ChartMode::smallerScaleMode() const {
  return nullptr;
}

DetailMode* ChartMode::largerScaleMode() const {
  const float hmm = m_camera->heightMM();
  const float wmm = hmm * m_camera->aspect();
  auto p = GeoProjection::CreateProjection(m_camera->geoprojection()->className());
  DetailMode* outlines = new OutlineMode(wmm, hmm, p);
  const quint32 scale = qMin(outlines->camera()->maxScale(), qMax(m_camera->scale(), outlines->camera()->minScale()));
  outlines->camera()->setScale(scale);
  outlines->camera()->reset(m_camera->eye(), m_camera->northAngle());
  return outlines;
}

Camera* ChartMode::cloneCamera() const {
  const float hmm = m_camera->heightMM();
  const float wmm = hmm * m_camera->aspect();
  GeoProjection* p = GeoProjection::CreateProjection(m_camera->geoprojection()->className());
  Camera* cam = new OrthoCam(wmm, hmm, p);
  cam->setScale(m_camera->scale());
  cam->reset(m_camera->eye(), m_camera->northAngle());
  return cam;
}

