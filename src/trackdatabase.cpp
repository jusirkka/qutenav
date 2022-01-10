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
               "enabled integer not null, " // boolean
               "preference integer not null)"); // provides sorting order

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

  // check validity
  auto it = std::find_if(events.cbegin(), events.cend(), [] (const KV::EventString& string) {
    return string.size() >= 2;
  });

  if (it == events.cend()) return;

  QString name = "Track 1";
  // find largest primary key
  auto r0 = exec("select max(id) from tracks");
  if (r0.first()) {
    name = QString("Track %1").arg(r0.value(0).toUInt() + 1);
  }

  quint32 pr = 1;
  // find largest preference
  r0 = exec("select max(preference) from tracks");
  if (r0.first()) {
    pr = r0.value(0).toUInt() + 1;
  }

  if (!transaction()) {
    qWarning() << "Transactions not supported";
  }


  r0 = prepare("insert into tracks "
               "(name, enabled, preference) "
               "values(?, ?, ?)");
  r0.bindValue(0, name);
  r0.bindValue(1, 0);
  r0.bindValue(1, pr);
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

KV::EventStringVector TrackDatabase::events(quint32 id) {

  KV::EventStringVector evs;

  try {


    auto r0 = prepare("select e.string_id, e.time, e.lng, e.lat from events e "
                      "join strings s on e.string_id = s.id "
                      "where s.track_id = ? "
                      "order by e.string_id, e.id");
    r0.bindValue(0, id);
    exec(r0);

    int prev = - 1;
    while (r0.next()) {

      const auto string_id = r0.value(0).toInt();
      if (string_id != prev) {
        evs << KV::EventString();
      }
      prev = string_id;

      auto wp = WGS84Point::fromLL(r0.value(2).toReal(), r0.value(3).toReal());
      evs.last() << KV::Event(r0.value(1).toLongLong(), wp);
    }

  } catch (DatabaseError& e) {
    qWarning() << e.msg();
  }

  return evs;

}

void TrackDatabase::remove(quint32 id) {
  try {

    if (!transaction()) {
      qWarning() << "Transactions not supported";
    }

    auto r0 = prepare("select e.id "
                      "from events e "
                      "join strings s on e.string_id = s.id "
                      "where s.track_id = ?");
    r0.bindValue(0, id);
    exec(r0);
    QVector<quint32> eids;
    while (r0.next()) {
      eids << r0.value(0).toUInt();
    }
    auto sql = QString("delete from events where id in (");
    sql += QString("?,").repeated(eids.size());
    sql = sql.replace(sql.length() - 1, 1, ")");
    r0 = prepare(sql);
    int index = 0;
    for (auto eid: eids) {
      r0.bindValue(index, eid);
      index++;
    }
    exec(r0);

    r0 = prepare("delete from strings where track_id = ?");
    r0.bindValue(0, id);
    exec(r0);

    r0 = prepare("delete from tracks where id = ?");
    r0.bindValue(0, id);
    exec(r0);

    if (!commit()) {
      qWarning() << "Transactions/Commits not supported";
    }
  } catch (DatabaseError& e) {
    qWarning() << e.msg();
    if (!rollback()) {
      qWarning() << "Transactions/Commits/Rollbacks not supported";
    }
  }
}




