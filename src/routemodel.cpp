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
#include "sqlitedatabase.h"


RouteModel::RouteModel(QObject *parent)
  : DatabaseModel(parent, 0)
{
  auto db = QSqlDatabase::addDatabase("QSQLITE", "RouteModel");
  db.setDatabaseName(SQLiteDatabase::databaseName("routes"));
  db.open();

  m_model = new QSqlTableModel(nullptr, db);
  m_model->setEditStrategy(QSqlTableModel::OnFieldChange);
  m_model->setTable("routes");
  m_model->setSort(m_sortColumn, Qt::DescendingOrder);
  m_model->select();
}
