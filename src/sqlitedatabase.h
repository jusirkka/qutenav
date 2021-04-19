/* -*- coding: utf-8-unix -*-
 *
 * sqlitedatabase.h
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
#pragma once

#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>


class SQLiteDatabase {
public:

  static QString databaseName(const QString& bname);

  SQLiteDatabase(const QString& connName);
  virtual ~SQLiteDatabase();

  const QSqlQuery& exec(const QString& sql);
  const QSqlQuery& prepare(const QString& sql);
  void exec(QSqlQuery& query);
  bool transaction();
  bool commit();
  void close();


  QString path() const {return m_DB.databaseName();}
  QSqlDatabase connection() const {return m_DB;}

protected:

  void checkError() const;

  QSqlDatabase m_DB;
  QSqlQuery m_Query;
};

