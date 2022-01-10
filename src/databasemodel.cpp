/* -*- coding: utf-8-unix -*-
 *
 * databasemodel.cpp
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
#include "databasemodel.h"

#include <QSqlRecord>
#include <QDebug>
#include "trackdatabase.h"




DatabaseModel::DatabaseModel(QObject *parent, int sortColumn)
  : QAbstractListModel(parent)
  , m_model(nullptr)
  , m_sortColumn(sortColumn)
{}



int DatabaseModel::rowCount(const QModelIndex&) const {
  return m_model->rowCount(QModelIndex());
}

Qt::ItemFlags DatabaseModel::flags(const QModelIndex& index) const {
  Qt::ItemFlags defaultFlags = QAbstractListModel::flags(index);
  if (!index.isValid()) return defaultFlags;
  return defaultFlags | Qt::ItemIsEditable;
}

DatabaseModel::~DatabaseModel() {
  m_model->database().close();
  delete m_model;
}



QHash<int, QByteArray> DatabaseModel::roleNames() const {
  QHash<int, QByteArray> roles;
  // record() returns an empty QSqlRecord
  for (int i = 0; i < m_model->record().count(); i++) {
    roles.insert(Qt::UserRole + 1 + i, m_model->record().fieldName(i).toUtf8());
  }
  return roles;
}

QVariant DatabaseModel::data(const QModelIndex& index, int role) const {
  if (!index.isValid()) return QVariant();

  // support only user roles = table columns
  if (role < Qt::UserRole + 1) {
    qDebug() << "DatabaseModel::data: role =" << role << Qt::UserRole;
    return QVariant();
  }

  int columnIdx = role - Qt::UserRole - 1;
  QModelIndex modelIndex = m_model->index(index.row(), columnIdx);
  return m_model->data(modelIndex, Qt::DisplayRole);
}

QVariant DatabaseModel::get(const QByteArray& roleName, int row) const {
  const int role = roleNames().key(roleName);
  const auto idx = index(row);
  return data(idx, role);
}

bool DatabaseModel::setData(const QModelIndex& item, const QVariant &value, int role) {
  if (!item.isValid()) return false;

  // support only user roles = table columns
  if (role < Qt::UserRole + 1) {
    qDebug() << "DatabaseModel::setData: role =" << role << Qt::UserRole;
    return false;
  }
  int columnIdx = role - Qt::UserRole - 1;
  if (columnIdx == 0) {
    qWarning() << "DatabaseModel::setData: cannot set autoincremented primary key";
    return false;
  }
  QModelIndex modelIndex = m_model->index(item.row(), columnIdx);
  bool ret = m_model->setData(modelIndex, value, Qt::EditRole);
  if (ret) {
      emit dataChanged(item, item);
  }
  return ret;
}

void DatabaseModel::sort() {
  beginResetModel();
  m_model->sort(m_sortColumn, Qt::DescendingOrder);
  endResetModel();
}

