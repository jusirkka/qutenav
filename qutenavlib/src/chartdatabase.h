/* -*- coding: utf-8-unix -*-
 *
 * chartdatabase.h
 *
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

#include "sqlitedatabase.h"

class WGS84Point;

class ChartDatabase: public SQLiteDatabase {
public:

  ChartDatabase();
  ChartDatabase(const QString& connName);
  ~ChartDatabase() = default;

  using ScaleVector = QVector<quint32>;

  void loadCharts(int chartset);
  QSqlQuery& charts(quint32 scale, const WGS84Point& sw0, const WGS84Point& ne0);
  QSqlQuery& scales(const ScaleVector& candidates, const WGS84Point& sw0, const WGS84Point& ne0);

  static void createTables();

private:

  static const inline QString boxConstraint =
      "(not iif(swx - :swx < 0, swx - :swx + 360, iif(swx - :swx >= 360, swx - :swx - 360, swx - :swx)) "
      "between :d0 and "
      "iif(nex - :swx < 0, nex - :swx + 360, iif(nex - :swx >= 360, nex - :swx - 360, nex - :swx))"
      ") and swy < :ney and ney > :swy";

  void bindBox(const WGS84Point& sw0, const WGS84Point& ne0);
};

