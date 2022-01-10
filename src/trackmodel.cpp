/* -*- coding: utf-8-unix -*-
 *
 * trackmodel.cpp
 *
 * Created: 13/04/2021 2021 by Jukka Sirkka
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
#include "trackmodel.h"
#include "trackdatabase.h"
#include "event.h"
#include <QDebug>


TrackModel::TrackModel(QObject *parent)
  : DatabaseModel(parent, preferenceColumn)
{
  QSqlDatabase db;
  const QString connName = "TrackModel";

  if (QSqlDatabase::contains(connName)) {
    db = QSqlDatabase::database(connName, false);
  } else {
    db = QSqlDatabase::addDatabase("QSQLITE", connName);
  }

  db.setDatabaseName(SQLiteDatabase::databaseName("tracks"));
  db.open();

  m_model = new QSqlTableModel(nullptr, db);
  m_model->setEditStrategy(QSqlTableModel::OnFieldChange);
  m_model->setTable("tracks");
  m_model->setSort(m_sortColumn, Qt::DescendingOrder);
  m_model->select();
}

qreal TrackModel::distance(quint32 id) const {

  TrackDatabase db("TrackModel::distance");
  const KV::EventStringVector evs = db.events(id);

  qreal dist = 0;
  for (const KV::EventString& string: evs) {
    for (int i = 0; i < string.size() - 1; i++) {
      dist += (string[i + 1].position - string[i].position).meters();
    }
  }
  return dist;
}

qreal TrackModel::straightLineDistance(quint32 id) const {
  TrackDatabase db("TrackModel::straightLineDistance");
  const KV::EventStringVector evs = db.events(id);

  return (evs.last().last().position - evs.first().first().position).meters();
}


qint64 TrackModel::instant(quint32 id, qint32 index) const {

  TrackDatabase db("TrackModel::instant");
  const KV::EventStringVector evs = db.events(id);

  const quint32 cnt = std::accumulate(evs.cbegin(), evs.cend(), 0, [] (quint32 c, const KV::EventString& s) {
      return c + s.size();
  });

  qint32 pos = (cnt + index) % cnt;
  Q_ASSERT(pos >= 0);

  for (const KV::EventString& string: evs) {
    if (pos < string.size()) {
      return string[pos].instant;
    }
    pos -= string.size();
    Q_ASSERT(pos >= 0);
  }
  return 0;
}

QGeoCoordinate TrackModel::location(quint32 id, qint32 index) const {
  TrackDatabase db("TrackModel::location");
  const KV::EventStringVector evs = db.events(id);

  const quint32 cnt = std::accumulate(evs.cbegin(), evs.cend(), 0, [] (quint32 c, const KV::EventString& s) {
      return c + s.size();
  });

  qint32 pos = (cnt + index) % cnt;
  Q_ASSERT(pos >= 0);

  for (const KV::EventString& string: evs) {
    if (pos < string.size()) {
      const WGS84Point& p = string[pos].position;
      return QGeoCoordinate(p.lat(), p.lng());
    }
    pos -= string.size();
    Q_ASSERT(pos >= 0);
  }
  return QGeoCoordinate();
}

quint32 TrackModel::topPreference() const {
  TrackDatabase db("TrackModel::topPreference");
  auto r0 = db.exec("select max(preference) from tracks");
  if (r0.first()) {
    return r0.value(0).toUInt();
  }
  return 0;
}

void TrackModel::remove(quint32 id) {
  beginResetModel();
  TrackDatabase db("TrackModel::remove");
  db.remove(id);
  m_model->sort(m_sortColumn, Qt::DescendingOrder);
  endResetModel();
}


qreal TrackModel::bearing(quint32 id) const {
  TrackDatabase db("TrackModel::bearing");
  const KV::EventStringVector evs = db.events(id);

  return (evs.last().last().position - evs.first().first().position).degrees();
}

qreal TrackModel::duration(quint32 id) const {
  TrackDatabase db("TrackModel::duration");
  const KV::EventStringVector evs = db.events(id);

  return .001 * (evs.last().last().instant - evs.first().first().instant);
}

qreal TrackModel::pausedDuration(quint32 id) const {
  TrackDatabase db("TrackModel::pausedDuration");
  const KV::EventStringVector evs = db.events(id);
  qint64 paused = 0;
  for (int i = 0; i < evs.size() - 1; i++) {
    paused += evs[i + 1].first().instant - evs[i].last().instant;
  }
  return .001 * paused;
}

qreal TrackModel::speed(quint32 id) const {
  TrackDatabase db("TrackModel::speed");
  const KV::EventStringVector evs = db.events(id);

  qreal dist = 0;
  for (const KV::EventString& string: evs) {
    for (int i = 0; i < string.size() - 1; i++) {
      dist += (string[i + 1].position - string[i].position).meters();
    }
  }

  return 1.e3 * dist / (evs.last().last().instant - evs.first().first().instant);
}

qreal TrackModel::speedWhileMoving(quint32 id) const {
  TrackDatabase db("TrackModel::speedWhileMoving");
  const KV::EventStringVector evs = db.events(id);

  qreal dist = 0;
  for (const KV::EventString& string: evs) {
    for (int i = 0; i < string.size() - 1; i++) {
      dist += (string[i + 1].position - string[i].position).meters();
    }
  }

  qint64 paused = 0;
  for (int i = 0; i < evs.size() - 1; i++) {
    paused += evs[i + 1].first().instant - evs[i].last().instant;
  }
  const qint64 dur = evs.last().last().instant - evs.first().first().instant - paused;

  return 1.e3 * dist / dur;
}

qreal TrackModel::straightLineSpeed(quint32 id) const {
  TrackDatabase db("TrackModel::straightLineSpeed");
  const KV::EventStringVector evs = db.events(id);

  const qreal dist = (evs.last().last().position - evs.first().first().position).meters();
  const qint64 dur = (evs.last().last().instant - evs.first().first().instant);

  return 1.e3 * dist / dur;
}

qreal TrackModel::maxSpeed(quint32 id) const {
  TrackDatabase db("TrackModel::maxSpeed");
  const KV::EventStringVector evs = db.events(id);

  qreal mspeed = 0;
  for (const KV::EventString& string: evs) {
    for (int i = 0; i < string.size() - 1; i++) {
      const qreal dist = (string[i + 1].position - string[i].position).meters();
      const qint64 dur = string[i + 1].instant - string[i].instant;
      const qreal speed = 1.e3 * dist / dur;
      if (speed > mspeed) {
        mspeed = speed;
      }
    }
  }

  return mspeed;
}

