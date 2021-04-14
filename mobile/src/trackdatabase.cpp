/* -*- coding: utf-8-unix -*-
 *
 * trackdatabase.cpp
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
#include "trackdatabase.h"

#include <QStandardPaths>
#include <QDir>
#include "types.h"
#include <QDebug>
#include <QDateTime>
#include <QSqlError>

QString TrackDatabase::databaseName() {
  // qopencpn or harbour-qopencpn
  const QString baseapp = qAppName();
  QString loc = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
  loc = QString("%1/%2/tracks").arg(loc).arg(baseapp);
  if (!QDir().mkpath(loc)) {
    throw ChartFileError(QString("cannot create tracks directory %1").arg(loc));
  }
  return QString("%1/tracks.db").arg(loc);
}


void TrackDatabase::createTables() {
  auto db = QSqlDatabase::addDatabase("QSQLITE");

  db.setDatabaseName(databaseName());
  db.open();
  auto query = QSqlQuery(db);

  query.exec("create table if not exists tracks ("
             "id integer primary key autoincrement, "
             "name text not null, "
             "enabled integer not null)"); // boolean

  query.exec("create table if not exists strings ("
             "id integer primary key autoincrement, "
             "track_id integer not null)");

  query.exec("create table if not exists events ("
              "id integer primary key autoincrement, "
              "string_id integer not null, "
              "time integer not null, " // secs since epoch
              "lng real not null, "
              "lat real not null)");

  db.close();
}

TrackDatabase::TrackDatabase()
  : SQLiteDatabase("TrackDatabase")
{
  m_DB.setDatabaseName(databaseName());
  m_DB.open();
  m_Query = QSqlQuery(m_DB);
}

void TrackDatabase::createTrack(const InstantVector& instants,
                                const WGS84PointVector& positions,
                                const GL::IndexVector& indices) {

  if (indices.isEmpty()) return;

  if (!transaction()) {
    qWarning() << "Transactions not supported";
  }

  auto name = QDateTime::fromMSecsSinceEpoch(instants.first() * 1000).toString("yyyy-MM-dd");

  auto r0 = prepare("insert into tracks "
                    "(name, enabled) "
                    "values(?, ?)");
  r0.bindValue(0, name);
  r0.bindValue(1, 0);
  exec(r0);

  auto track_id = r0.lastInsertId().toUInt();

  int i = 0;
  while (i < indices.size()) {
    EventVector string;
    string << Event(instants[indices[i]], positions[indices[i]]);
    i++;
    while (i < indices.size() - 1 && indices[i + 1] == indices[i]) {
      string << Event(instants[indices[i]], positions[indices[i]]);
      i += 2;
    }
    string << Event(instants[indices[i]], positions[indices[i]]);
    i++;
    auto r1 = prepare("insert into strings "
                      "(track_id) "
                      "values(?)");
    r1.bindValue(0, track_id);
    exec(r1);
    auto string_id = r1.lastInsertId().toUInt();

    auto r2 = prepare("insert into events "
                      "(string_id, time, lng, lat) "
                      "values (?, ?, ?, ?)");
    for (const Event& ev: string) {
      r2.bindValue(0, string_id);
      r2.bindValue(1, ev.instant);
      r2.bindValue(2, ev.position.lng());
      r2.bindValue(3, ev.position.lat());
      exec(r2);
    }
  }

  if (!commit()) {
    qWarning() << "Transactions/Commits not supported";
  }

}
