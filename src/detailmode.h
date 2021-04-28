/* -*- coding: utf-8-unix -*-
 *
 * File: src/detailmode.h
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

#include "camera.h"
#include "drawable.h"

class DetailMode: public QObject {

  Q_OBJECT

public:

  DetailMode(QObject* parent = nullptr);

  static DetailMode* RestoreState();
  static Camera* RestoreCamera();
  Camera* camera() {return m_camera;}
  const Camera* camera() const {return m_camera;}
  DrawableVector& drawables() {return m_drawables;}
  virtual DetailMode* largerScaleMode() const = 0;
  virtual DetailMode* smallerScaleMode() const = 0;
  virtual Camera* cloneCamera() const = 0;
  virtual ~DetailMode() = default;

  void saveState() const;

protected:

  virtual QString className() const = 0;

  Camera* m_camera;
  DrawableVector m_drawables;

};


class ModeError {
public:
  ModeError(QString msg): m_detail(std::move(msg)) {}
  const QString msg() const {return m_detail;}
private:
  QString m_detail;
};
