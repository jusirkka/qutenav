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

#include "types.h"
#include <QDebug>
#include <QDateTime>
#include <QSqlError>


void TrackDatabase::createTables() {
  {
    auto db = QSqlDatabase::addDatabase("QSQLITE", "TrackDatabase::createTables");

    db.setDatabaseName(databaseName("tracks"));
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
               "time integer not null, " // millisecs since epoch
               "lng real not null, "
               "lat real not null)");

    db.close();
  }
  QSqlDatabase::removeDatabase("TrackDatabase::createTables");
}

TrackDatabase::TrackDatabase(const QString& connName)
  : SQLiteDatabase(connName)
{
  m_DB.setDatabaseName(databaseName("tracks"));
  m_DB.open();
  m_Query = QSqlQuery(m_DB);
}

void TrackDatabase::createTrack(const KV::EventStringVector& events) {

  // check consistency & set name
  QString name;
  for (const KV::EventString& string: events) {
    if (string.size() >= 2) {
      name = QDateTime::fromMSecsSinceEpoch(string.first().instant).toString("yyyy-MM-dd");
      break;
    }
  }

  if (name.isEmpty()) return;


  if (!transaction()) {
    qWarning() << "Transactions not supported";
  }


  auto r0 = prepare("insert into tracks "
                    "(name, enabled) "
                    "values(?, ?)");
  r0.bindValue(0, name);
  r0.bindValue(1, 0);
  exec(r0);

  auto track_id = r0.lastInsertId().toUInt();

  for (const KV::EventString& string: events) {
    if (string.size() < 2) continue;
    auto r1 = prepare("insert into strings "
                      "(track_id) "
                      "values(?)");
    r1.bindValue(0, track_id);
    exec(r1);
    auto string_id = r1.lastInsertId().toUInt();


    auto r2 = prepare("insert into events "
                      "(string_id, time, lng, lat) "
                      "values (?, ?, ?, ?)");
    for (const KV::Event& ev: string) {
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
