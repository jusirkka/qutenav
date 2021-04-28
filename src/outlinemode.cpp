/* -*- coding: utf-8-unix -*-
 *
 * File: src/outlinemode.cpp
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
#include "outlinemode.h"
#include "perscam.h"
#include "globe.h"
#include "outliner.h"
#include "chartmode.h"

OutlineMode::OutlineMode(float wmm, float hmm, GeoProjection* p)
  : DetailMode()
{
  m_camera = new PersCam(wmm, hmm, p);
  m_drawables << new Globe(this) << new Outliner(this);
}

OutlineMode::~OutlineMode() {
  delete m_camera;
  qDeleteAll(m_drawables);
}

DetailMode* OutlineMode::largerScaleMode() const {
  return nullptr;
}

DetailMode* OutlineMode::smallerScaleMode() const {
  const float hmm = m_camera->heightMM();
  const float wmm = hmm * m_camera->aspect();
  auto p = GeoProjection::CreateProjection(m_camera->geoprojection()->className());
  DetailMode* charts = new ChartMode(wmm, hmm, p);
  const quint32 scale = qMin(m_camera->scale(), charts->camera()->maxScale());
  charts->camera()->setScale(scale);
  charts->camera()->reset(m_camera->eye(), m_camera->northAngle());
  return charts;
}

Camera* OutlineMode::cloneCamera() const {
  const float hmm = m_camera->heightMM();
  const float wmm = hmm * m_camera->aspect();
  GeoProjection* p = GeoProjection::CreateProjection(m_camera->geoprojection()->className());
  Camera* cam = new PersCam(wmm, hmm, p);
  cam->setScale(m_camera->scale());
  cam->reset(m_camera->eye(), m_camera->northAngle());
  return cam;
}

