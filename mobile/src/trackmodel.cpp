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
#include <QSqlRecord>
#include <QDebug>
#include "trackdatabase.h"




TrackModel::TrackModel(QObject *parent)
  : QAbstractListModel(parent)
  , m_tracks(nullptr)
{
  auto db = QSqlDatabase::addDatabase("QSQLITE", "TrackModel");
  db.setDatabaseName(TrackDatabase::databaseName());
  db.open();

  m_tracks = new QSqlTableModel(nullptr, db);
  m_tracks->setEditStrategy(QSqlTableModel::OnFieldChange);
  m_tracks->setTable("tracks");
  m_tracks->select();
}



int TrackModel::rowCount(const QModelIndex&) const {
  return m_tracks->rowCount(QModelIndex());
}

Qt::ItemFlags TrackModel::flags(const QModelIndex& index) const {
  Qt::ItemFlags defaultFlags = QAbstractListModel::flags(index);
  if (!index.isValid()) return defaultFlags;
  return defaultFlags | Qt::ItemIsEditable;
}

TrackModel::~TrackModel() {
  m_tracks->database().close();
  const auto connName = m_tracks->database().connectionName();
  delete m_tracks;
  QSqlDatabase::removeDatabase(connName);
}



QHash<int, QByteArray> TrackModel::roleNames() const {
  QHash<int, QByteArray> roles;
  // record() returns an empty QSqlRecord. Skip primary key
  for (int i = 1; i < m_tracks->record().count(); i ++) {
    roles.insert(Qt::UserRole + i, m_tracks->record().fieldName(i).toUtf8());
  }
  return roles;
}

QVariant TrackModel::data(const QModelIndex& index, int role) const {
  if (!index.isValid()) return QVariant();

  // support only user roles = table columns
  if (role < Qt::UserRole + 1) {
    qDebug() << "TrackModel::data: role =" << role << Qt::UserRole;
    return QVariant();
  }

  int columnIdx = role - Qt::UserRole;
  QModelIndex modelIndex = m_tracks->index(index.row(), columnIdx);
  return m_tracks->data(modelIndex, Qt::DisplayRole);
}


bool TrackModel::setData(const QModelIndex& item, const QVariant &value, int role) {
  if (!item.isValid()) return false;

  // support only user roles = table columns
  if (role < Qt::UserRole + 1) {
    qDebug() << "TrackModel::setData: role =" << role << Qt::UserRole;
    return false;
  }
  int columnIdx = role - Qt::UserRole;
  QModelIndex modelIndex = m_tracks->index(item.row(), columnIdx);
  bool ret = m_tracks->setData(modelIndex, value, Qt::EditRole);
  if (ret) {
    emit dataChanged(item, item);
  }
  return ret;
}
