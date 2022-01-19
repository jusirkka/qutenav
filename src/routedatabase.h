/* -*- coding: utf-8-unix -*-
 *
 * routedatabase.h
 *
 * Created: 18/04/2021 2021 by Jukka Sirkka
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
#include "types.h"

class RouteDatabase: public SQLiteDatabase {
public:

  static void createTables();

  RouteDatabase(const QString& connName);
  ~RouteDatabase() = default;

  int createRoute(const WGS84PointVector& wps);
  void modifyRoute(int rid, const WGS84PointVector& wps);
  void deleteRoute(int rid);

  WGS84PointVector wayPoints(quint32 id);

private:

};

