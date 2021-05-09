/* -*- coding: utf-8-unix -*-
 *
 * trackdatabase.h
 *
 * Created: 12/04/2021 2021 by Jukka Sirkka
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

using InstantVector = QVector<qint64>; // UTC unix times in millisecs

class TrackDatabase: public SQLiteDatabase {
public:

  static void createTables();

  TrackDatabase(const QString& connName);
  ~TrackDatabase() = default;

  void createTrack(const InstantVector& events, const WGS84PointVector& positions, const GL::IndexVector& indices);

private:

  struct Event {
    Event(qint64 t, const WGS84Point& p)
      : instant(t)
      , position(p) {}

    Event() = default;

    quint64 instant;
    WGS84Point position;
  };

  using EventVector = QVector<Event>;
};

