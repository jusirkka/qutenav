/* -*- coding: utf-8-unix -*-
 *
 * routemodel.h
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

#include "databasemodel.h"
#include <QGeoCoordinate>

class RouteModel: public DatabaseModel {

  Q_OBJECT

public:

  RouteModel(QObject* parent = nullptr);
  ~RouteModel() = default;

  Q_INVOKABLE qreal distance(quint32 id) const; // meters
  Q_INVOKABLE int wayPointCount(quint32 id) const;
  Q_INVOKABLE QGeoCoordinate location(quint32 id, qint32 index) const;
  Q_INVOKABLE quint32 topPreference() const;
  Q_INVOKABLE void remove(quint32 id);


private:

  static const int preferenceColumn = 2;

};
