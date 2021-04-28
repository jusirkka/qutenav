/* -*- coding: utf-8-unix -*-
 *
 * File: src/orthocam.h
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
#include "camera.h"

class OrthoCam: public Camera {

public:

  OrthoCam(float wmm, float hmm, GeoProjection* proj);
  OrthoCam(const QSizeF& vp, const WGS84Point& eye, quint32 scale, GeoProjection* proj);

  void pan(const QPointF& dragStart, const QPointF& dragAmount) override;
  WGS84Point location(const QPointF &cp) const override;
  QPointF position(const WGS84Point &wp) const override;
  void rotateEye(const Angle& a) override;
  void setScale(quint32 scale) override;
  void resize(float wmm, float hmm) override;
  void reset() override;
  void reset(const WGS84Point& eye, const Angle& tilt) override;
  void setEye(const WGS84Point& eye) override;
  WGS84Point eye() const override;
  Angle northAngle() const override;
  quint32 maxScale() const override;
  quint32 minScale() const override;
  QRectF boundingBox() const override;

protected:

  void doUpdate(const Camera *other) override;

private:

  void updateProjection();
  void doReset();

  const quint32 MAX_SCALE = UINT32_MAX;
  const quint32 MIN_SCALE = 800;

  WGS84Point m_reference_0;
  Angle m_northAngle;
  Angle m_northAngle_0;
};
