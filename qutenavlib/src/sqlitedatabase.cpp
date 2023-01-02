/* -*- coding: utf-8-unix -*-
 *
 * sqlitedatabase.cpp
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
#include "sqlitedatabase.h"

#include "types.h"
#include <QDebug>
#include <QtSql/QSqlError>
#include <QStandardPaths>
#include <QDir>
#include "platform.h"

QString SQLiteDatabase::databaseName(const QString& bname) {
  QString loc = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
  loc = QString("%1/%2/userdata").arg(loc).arg(Platform::base_app_name());
  if (!QDir().mkpath(loc)) {
    throw ChartFileError(QString("cannot create userdata directory %1").arg(loc));
  }
  return QString("%1/%2.db").arg(loc).arg(bname);
}



SQLiteDatabase::SQLiteDatabase(const QString& connName)
{
  if (QSqlDatabase::contains(connName)) {
    m_DB = QSqlDatabase::database(connName, false);
  } else {
    m_DB = QSqlDatabase::addDatabase("QSQLITE", connName);
  }
}


const QSqlQuery& SQLiteDatabase::exec(const QString& sql) {
  m_Query = QSqlQuery(m_DB);

  m_Query.exec(sql);
  checkError();

  return m_Query;
}

void SQLiteDatabase::exec(QSqlQuery& query) {
  m_Query = query;
  m_Query.exec();
  checkError();
}

const QSqlQuery& SQLiteDatabase::prepare(const QString& sql) {
  m_Query = QSqlQuery(m_DB);
  m_Query.prepare(sql);
  checkError();

  return m_Query;
}

bool SQLiteDatabase::transaction() {
  return m_DB.transaction();
}

bool SQLiteDatabase::commit() {
  return m_DB.commit();
}

bool SQLiteDatabase::rollback() {
  return m_DB.rollback();
}

void SQLiteDatabase::close() {
  m_DB.commit();
  m_DB.close();
}

SQLiteDatabase::~SQLiteDatabase() {
  // qDebug() << "SQLiteDatabase::~SQLiteDatabase" << m_DB.connectionName();
  close();
}

void SQLiteDatabase::checkError() const {
  if (!m_Query.lastError().isValid()) return;
  throw DatabaseError(m_Query.lastError().text());
}
