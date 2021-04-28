/* -*- coding: utf-8-unix -*-
 *
 * File: src/perscam.h
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

class PersCam: public Camera {

public:

  PersCam(float wmm, float hmm, GeoProjection* p);
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

  ~PersCam() = default;


private:

  float eyeDistFromScale() const;
  void setScaleFromEyeDist();
  void setEyeDist();

  void setEyeAndTilt(const WGS84Point& eye, const Angle& tilt);

protected:

  void doUpdate(const Camera *other) override;

private:


  const float EPSILON = 1.e-5;
  const float R0 = 6371008.8;
  const float PI = 3.14159265358979323846;
  const float FOV2 = PI / 8;
  const quint32 MIN_SCALE = 20000;

  QMatrix4x4 m_rot, m_rot0;
  QVector3D m_eye, m_eye0;
};
