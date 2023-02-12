/* -*- coding: utf-8-unix -*-
 *
 * File: src/chartdatabase.cpp
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
#include "chartdatabase.h"
#include <QStandardPaths>
#include <QDir>
#include "types.h"
#include <QDebug>

ChartDatabase::ChartDatabase()
  : SQLiteDatabase("manager")
{
  m_DB.setDatabaseName(databaseName("charts"));
  m_DB.open();
  m_Query = QSqlQuery(m_DB);

  m_Query.exec("attach database ':memory:' as m");
  checkError();

  m_Query.exec("create table if not exists m.charts ("
               "chart_id integer not null, "
               "scale integer not null, "
               "priority integer not null, "
               "swx real not null, "
               "swy real not null, "
               "nex real not null, "
               "ney real not null, "
               "path text not null unique)");
  checkError();

  m_Query.exec("create table if not exists m.coverage ("
               "id integer not null, "
               "type_id integer not null, "
               "chart_id integer not null)");
  checkError();

  m_Query.exec("create table if not exists m.polygons ("
               "id integer not null, "
               "cov_id integer not null, "
               "x real not null, "
               "y real not null)");
  checkError();
}

ChartDatabase::ChartDatabase(const QString& connName)
  : SQLiteDatabase(connName)
{
  m_DB.setDatabaseName(databaseName("charts"));
  m_DB.open();
  m_Query = QSqlQuery(m_DB);
}


void ChartDatabase::createTables() {
  {
    auto db = QSqlDatabase::addDatabase("QSQLITE", "ChartDatabase::createTables");

    db.setDatabaseName(databaseName("charts"));
    db.open();
    auto query = QSqlQuery(db);

    query.exec("create table if not exists chartsets ("
               "id integer primary key autoincrement, "
               "name text not null)");

    query.exec("create table if not exists scales ("
               "id integer primary key autoincrement, "
               "chartset_id integer not null, "
               "scale int not null, "
               "priority int not null, "
               "unique (chartset_id, scale))");

    query.exec("create table if not exists charts ("
               "id integer primary key autoincrement, "
               "scale_id integer not null, "
               "swx real not null, "
               "swy real not null, "
               "nex real not null, "
               "ney real not null, "
               "published int not null, " // Julian day
               "modified int not null, "  // Julian day
               "path text not null unique)");

    query.exec("create table if not exists coverage ("
               "id integer primary key autoincrement, "
               "type_id integer not null, "
               "chart_id integer not null)");

    query.exec("create table if not exists polygons ("
               "id integer primary key autoincrement, "
               "cov_id integer not null, "
               "x real not null, "
               "y real not null)");

    db.close();
  }
  QSqlDatabase::removeDatabase("ChartDatabase::createTables");
}

QSqlQuery& ChartDatabase::scales(const ScaleVector& candidates,
                                 const WGS84Point& sw0,
                                 const WGS84Point& ne0) {

  // assuming a) candidates non-empty and b) in ascending order

  m_Query = QSqlQuery(m_DB);
  m_Query.prepare(QString("select distinct scale, priority "
                          "from m.charts "
                          "where scale < :maxscale and %1 "
                          "order by scale desc")
                  .arg(boxConstraint));
  m_Query.bindValue(":maxscale", candidates.last());
  bindBox(sw0, ne0);

  m_Query.exec();

  return m_Query;
}

QSqlQuery& ChartDatabase::charts(quint32 scale,
                                 const WGS84Point& sw0,
                                 const WGS84Point& ne0) {

  m_Query = QSqlQuery(m_DB);
  m_Query.prepare(QString("select chart_id, swx, swy, nex, ney, path "
                          "from m.charts "
                          "where scale = :scale and %1")
                  .arg(boxConstraint));

  m_Query.bindValue(":scale", scale);
  bindBox(sw0, ne0);

  m_Query.exec();

  return m_Query;
}

static double diff(double x1, double x2) {
  auto d = x2 - x1;
  while (d < 0.) d += 360.;
  while (d >= 360.) d -= 360.;
  return d;
}

void ChartDatabase::bindBox(const WGS84Point& sw0, const WGS84Point& ne0) {
  m_Query.bindValue(":d0", diff(sw0.lng(), ne0.lng()));
  m_Query.bindValue(":swx", sw0.lng());
  m_Query.bindValue(":swy", sw0.lat());
  m_Query.bindValue(":ney", ne0.lat());
}



void ChartDatabase::loadCharts(int chartset_id) {
  m_Query = QSqlQuery(m_DB);

  m_Query.exec("delete from m.charts");
  checkError();
  m_Query.exec("delete from m.coverage");
  checkError();
  m_Query.exec("delete from m.polygons");
  checkError();

  m_Query.prepare("insert into m.charts select "
                  "c.id, s.scale, s.priority, c.swx, c.swy, c.nex, c.ney, c.path from "
                  "main.charts c "
                  "join main.scales s on c.scale_id = s.id "
                  "where s.chartset_id = ?");
  checkError();

  m_Query.bindValue(0, chartset_id);
  m_Query.exec();
  checkError();

  m_Query.prepare("insert into m.coverage select "
                  "v.id, v.type_id, v.chart_id from "
                  "main.charts c "
                  "join main.scales s on c.scale_id = s.id "
                  "join main.coverage v on v.chart_id = c.id "
                  "where s.chartset_id = ?");
  checkError();

  m_Query.bindValue(0, chartset_id);
  m_Query.exec();
  checkError();

  m_Query.prepare("insert into m.polygons select "
                  "p.id, p.cov_id, p.x, p.y from "
                  "main.charts c "
                  "join main.scales s on c.scale_id = s.id "
                  "join main.coverage v on v.chart_id = c.id "
                  "join main.polygons p on p.cov_id = v.id "
                  "where s.chartset_id = ?");
  checkError();

  m_Query.bindValue(0, chartset_id);
  m_Query.exec();
  checkError();
}
