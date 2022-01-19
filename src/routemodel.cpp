/* -*- coding: utf-8-unix -*-
 *
 * routemodel.cpp
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
#include "routemodel.h"
#include "routedatabase.h"


RouteModel::RouteModel(QObject *parent)
  : DatabaseModel(parent, preferenceColumn)
{
  QSqlDatabase db;
  const QString connName = "RouteModel";

  if (QSqlDatabase::contains(connName)) {
    db = QSqlDatabase::database(connName, false);
  } else {
    db = QSqlDatabase::addDatabase("QSQLITE", connName);
  }

  db.setDatabaseName(SQLiteDatabase::databaseName("routes"));
  db.open();

  m_model = new QSqlTableModel(nullptr, db);
  m_model->setEditStrategy(QSqlTableModel::OnFieldChange);
  m_model->setTable("routes");
  m_model->setSort(m_sortColumn, Qt::DescendingOrder);
  m_model->select();
}


qreal RouteModel::distance(quint32 id) const {
  RouteDatabase db("RouteModel::distance");
  const WGS84PointVector wps = db.wayPoints(id);

  qreal dist = 0;
  for (int i = 0; i < wps.size() - 1; i++) {
      dist += (wps[i + 1] - wps[i]).meters();
  }
  return dist;
}

int RouteModel::wayPointCount(quint32 id) const {
  RouteDatabase db("RouteModel::wayPointCount");
  const WGS84PointVector wps = db.wayPoints(id);
  return wps.size();
}

QGeoCoordinate RouteModel::location(quint32 id, qint32 index) const {
  RouteDatabase db("RouteModel::location");
  const WGS84PointVector wps = db.wayPoints(id);


  qint32 pos = (wps.size() + index) % wps.size();
  Q_ASSERT(pos >= 0);
  const WGS84Point& p = wps[pos];

  return QGeoCoordinate(p.lat(), p.lng());
}

quint32 RouteModel::topPreference() const {
  RouteDatabase db("RouteModel::topPreference");
  auto r0 = db.exec("select max(preference) from routes");
  if (r0.first()) {
    return r0.value(0).toUInt();
  }
  return 0;
}

void RouteModel::remove(quint32 id) {
  beginResetModel();
  RouteDatabase db("RouteDatabase::remove");
  db.deleteRoute(id);
  m_model->sort(m_sortColumn, Qt::DescendingOrder);
  endResetModel();
}
