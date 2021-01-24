#include "chartdatabase.h"
#include <QStandardPaths>
#include <QDir>
#include "types.h"
#include <QDebug>
#include <QtSql/QSqlError>

ChartDatabase::ChartDatabase()
  : m_DB(QSqlDatabase::addDatabase("QSQLITE"))
{
  QString loc = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
  loc = QString("%1/%2/charts").arg(loc).arg(qAppName());
  if (!QDir().mkpath(loc)) {
    throw ChartFileError(QString("cannot create charts directory %1").arg(loc));
  }
  const QString dbfile = QString("%1/charts.db").arg(loc);
  m_DB.setDatabaseName(dbfile);
  m_DB.open();
  m_Query = QSqlQuery(m_DB);

  m_Query.exec("create table if not exists chartsets ("
               "id integer primary key autoincrement, "
               "name text not null)");
  checkError();

  m_Query.exec("create table if not exists scales ("
               "id integer primary key autoincrement, "
               "chartset_id integer not null, "
               "scale int not null)");
  checkError();

  m_Query.exec("create table if not exists charts ("
               "id integer primary key autoincrement, "
               "scale_id integer not null, "
               "swx real not null, "
               "swy real not null, "
               "nex real not null, "
               "ney real not null, "
               "published int not null, " // Julian day
               "modified int not null, "  // Julian day
               "path text not null unique)");
  checkError();

  m_Query.exec("attach database ':memory:' as m");
  checkError();

  m_Query.exec("create table if not exists m.charts ("
               "chart_id integer not null, "
               "scale integer not null, "
               "swx real not null, "
               "swy real not null, "
               "nex real not null, "
               "ney real not null, "
               "path text not null unique)");
  checkError();
}

void ChartDatabase::loadCharts(int chartset_id) {
  m_Query = QSqlQuery(m_DB);

  m_Query.exec("delete from m.charts");
  checkError();

  m_Query.prepare("insert into m.charts select "
                  "c.id, s.scale, c.swx, c.swy, c.nex, c.ney, c.path from "
                  "main.charts c join main.scales s on c.scale_id = s.id where "
                  "s.chartset_id = ?");
  checkError();

  m_Query.bindValue(0, chartset_id);
  m_Query.exec();
  checkError();
}

const QSqlQuery& ChartDatabase::exec(const QString& sql) {
  m_Query = QSqlQuery(m_DB);

  m_Query.exec(sql);
  checkError();

  return m_Query;
}

void ChartDatabase::exec(QSqlQuery& query) {
  m_Query = query;
  m_Query.exec();
  checkError();
}

const QSqlQuery& ChartDatabase::prepare(const QString& sql) {
  m_Query = QSqlQuery(m_DB);
  m_Query.prepare(sql);
  checkError();

  return m_Query;
}

bool ChartDatabase::transaction() {
  return m_DB.transaction();
}

bool ChartDatabase::commit() {
  return m_DB.commit();
}

void ChartDatabase::close() {
  m_DB.commit();
  m_DB.close();
}

ChartDatabase::~ChartDatabase() {
  close();
}

void ChartDatabase::checkError() const {
  if (!m_Query.lastError().isValid()) return;
  throw DatabaseError(m_Query.lastError().text());
}
