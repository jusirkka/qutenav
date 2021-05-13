/* -*- coding: utf-8-unix -*-
 *
 * File: src/detailmode.cpp
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
#include "detailmode.h"
#include "chartmode.h"
#include "outlinemode.h"
#include "conf_detailmode.h"
#include "perscam.h"
#include "orthocam.h"

DetailMode::DetailMode(QObject *parent): QObject(parent) {}


Camera* DetailMode::RestoreCamera() {
  const float wmm = Conf::DetailMode::WidthMM();
  const float hmm = Conf::DetailMode::HeightMM();

  auto p = GeoProjection::CreateProjection(Conf::DetailMode::Projection());
  if (p == nullptr) {
    p = new SimpleMercator;
  }

  const QString name = Conf::DetailMode::Name();
  Camera* cam;
  if (name == "OutlineMode") {
    cam = new PersCam(wmm, hmm, p);
  } else if (name == "ChartMode") {
    cam = new OrthoCam(wmm, hmm, p);
  } else {
    throw ModeError("unsupported mode");
  }

  quint32 scale = Conf::DetailMode::Scale();
  scale = qMin(scale, cam->maxScale());
  scale = qMax(scale, cam->minScale());
  cam->setScale(scale);

  WGS84Point e = WGS84Point::parseISO6709(Conf::DetailMode::Eye());
  if (!e.valid()) e = cam->eye();

  const Angle a = Angle::fromDegrees(Conf::DetailMode::NorthAngle());

  cam->reset(e, a);

  return cam;
}

DetailMode* DetailMode::RestoreState() {

  const float wmm = Conf::DetailMode::WidthMM();
  const float hmm = Conf::DetailMode::HeightMM();

  auto p = GeoProjection::CreateProjection(Conf::DetailMode::Projection());
  if (p == nullptr) {
    p = new SimpleMercator;
  }

  const QString name = Conf::DetailMode::Name();
  DetailMode* mode;
  if (name == "OutlineMode") {
    mode = new OutlineMode(wmm, hmm, p);
  } else if (name == "ChartMode") {
    mode = new ChartMode(wmm, hmm, p);
  } else {
    throw ModeError("unsupported mode");
  }

  quint32 scale = Conf::DetailMode::Scale();
  scale = qMin(scale, mode->camera()->maxScale());
  scale = qMax(scale, mode->camera()->minScale());
  mode->camera()->setScale(scale);

  WGS84Point e = WGS84Point::parseISO6709(Conf::DetailMode::Eye());
  if (!e.valid()) e = mode->camera()->eye();

  const Angle a = Angle::fromDegrees(Conf::DetailMode::NorthAngle());

  mode->camera()->reset(e, a);

  return mode;
}

void DetailMode::saveState() const {
  Conf::DetailMode::setName(className());

  const float hmm = m_camera->heightMM();
  const float wmm = hmm * m_camera->aspect();
  Conf::DetailMode::setHeightMM(hmm);
  Conf::DetailMode::setWidthMM(wmm);

  Conf::DetailMode::setScale(m_camera->scale());

  Conf::DetailMode::setEye(m_camera->eye().toISO6709());

  Conf::DetailMode::setNorthAngle(m_camera->northAngle().degrees());

  Conf::DetailMode::setProjection(m_camera->geoprojection()->className());

  Conf::DetailMode::self()->save();

}
