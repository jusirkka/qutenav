/* -*- coding: utf-8-unix -*-
 *
 * File: src/s57chartoutline_p.h
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

#include <QSharedData>
#include <QPoint>
#include "types.h"
#include <QDate>

using Region = QVector<WGS84PointVector>;

struct S57ChartOutlinePrivate: public QSharedData {


  inline S57ChartOutlinePrivate()
  {}

  inline S57ChartOutlinePrivate(const S57ChartOutlinePrivate& d)
    : QSharedData(d)
    , extent(d.extent.sw(), d.extent.se(), d.extent.nw(), d.extent.ne())
    , cov(d.cov)
    , nocov(d.nocov)
    , center(d.center)
    , scaling(d.scaling)
    , scale(d.scale)
    , pub(d.pub)
    , mod(d.mod) {}

  Extent extent;
  Region cov;
  Region nocov;
  WGS84Point center;
  QSizeF scaling;
  quint32 scale;
  QDate pub;
  QDate mod;
};
