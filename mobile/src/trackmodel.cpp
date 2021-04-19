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
#include "sqlitedatabase.h"


TrackModel::TrackModel(QObject *parent)
  : DatabaseModel(parent)
{
  auto db = QSqlDatabase::addDatabase("QSQLITE", "TrackModel");
  db.setDatabaseName(SQLiteDatabase::databaseName("tracks"));
  db.open();

  m_model = new QSqlTableModel(nullptr, db);
  m_model->setEditStrategy(QSqlTableModel::OnFieldChange);
  m_model->setTable("tracks");
  m_model->setSort(0, Qt::DescendingOrder);
  m_model->select();
}
