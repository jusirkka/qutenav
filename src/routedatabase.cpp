/* -*- coding: utf-8-unix -*-
 *
 * routedatabase.cpp
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
#include "routedatabase.h"

#include <QDebug>
#include <QSqlError>


void RouteDatabase::createTables() {
  auto db = QSqlDatabase::addDatabase("QSQLITE");

  db.setDatabaseName(databaseName("routes"));
  db.open();
  auto query = QSqlQuery(db);

  query.exec("create table if not exists routes ("
             "id integer primary key autoincrement, "
             "name text not null)");

  query.exec("create table if not exists paths ("
             "id integer primary key autoincrement, "
             "route_id integer not null, "
             "lng real not null, "
             "lat real not null)");

  db.close();
  QSqlDatabase::removeDatabase(db.connectionName());
}

RouteDatabase::RouteDatabase(const QString& connName)
  : SQLiteDatabase(connName)
{
  m_DB.setDatabaseName(databaseName("routes"));
  m_DB.open();
  m_Query = QSqlQuery(m_DB);
}

int RouteDatabase::createRoute(const WGS84PointVector& wps) {

  if (!transaction()) {
    qWarning() << "Transactions not supported";
  }

  auto r0 = prepare("insert into routes "
                    "(name) "
                    "values(?)");
  r0.bindValue(0, "Route");
  exec(r0);

  auto route_id = r0.lastInsertId().toInt();

  auto r1 = prepare("update routes "
                    "set name = ? "
                    "where id = ?");
  r1.bindValue(0, QString("Route %1").arg(route_id));
  r1.bindValue(1, route_id);
  exec(r1);


  auto r2 = prepare("insert into paths "
                    "(route_id, lng, lat) "
                    "values (?, ?, ?)");

  for (const WGS84Point& wp: wps) {
    r2.bindValue(0, route_id);
    r2.bindValue(1, wp.lng());
    r2.bindValue(2, wp.lat());
    exec(r2);
  }

  if (!commit()) {
    qWarning() << "Transactions/Commits not supported";
  }

  return route_id;
}

void RouteDatabase::modifyRoute(int rid, const WGS84PointVector& wps) {

  if (!transaction()) {
    qWarning() << "Transactions not supported";
  }

  auto r0 = prepare("delete from paths "
                    "where route_id = ?");
  r0.bindValue(0, rid);
  exec(r0);


  auto r1 = prepare("insert into paths "
                    "(route_id, lng, lat) "
                    "values (?, ?, ?)");

  for (const WGS84Point& wp: wps) {
    r1.bindValue(0, rid);
    r1.bindValue(1, wp.lng());
    r1.bindValue(2, wp.lat());
    exec(r1);
  }

  if (!commit()) {
    qWarning() << "Transactions/Commits not supported";
  }
}

void RouteDatabase::deleteRoute(int rid) {

  if (!transaction()) {
    qWarning() << "Transactions not supported";
  }

  auto r0 = prepare("delete from routes "
                    "where id = ?");
  r0.bindValue(0, rid);
  exec(r0);


  auto r1 = prepare("delete from paths "
                    "where route_id = ?");
  r1.bindValue(0, rid);
  exec(r1);

  if (!commit()) {
    qWarning() << "Transactions/Commits not supported";
  }
}
