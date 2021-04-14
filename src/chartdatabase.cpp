#include "chartdatabase.h"
#include <QStandardPaths>
#include <QDir>
#include "types.h"
#include <QDebug>

ChartDatabase::ChartDatabase()
  : SQLiteDatabase("ChartDatabase")
{
  // qopencpn or harbour-qopencpn
  const QString baseapp = qAppName().split("_").first();
  QString loc = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
  loc = QString("%1/%2/charts").arg(loc).arg(baseapp);
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
               "scale int not null, "
               "unique (chartset_id, scale))");
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

  m_Query.exec("create table if not exists coverage ("
               "id integer primary key autoincrement, "
               "type_id integer not null, "
               "chart_id integer not null)");
  checkError();

  m_Query.exec("create table if not exists polygons ("
               "id integer primary key autoincrement, "
               "cov_id integer not null, "
               "x real not null, "
               "y real not null)");
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

  m_Query.exec("create table if not exists m.coverage ("
               "id integer not null, "
               "type_id integer not null, "
               "chart_id integer not null)");
  checkError();

  m_Query.exec("create table if not exists m.polygons ("
               "id integer not null, "
               "cov_id integer not null, "
               "x real not null, "
               "y real not null)");
  checkError();

}

void ChartDatabase::loadCharts(int chartset_id) {
  m_Query = QSqlQuery(m_DB);

  m_Query.exec("delete from m.charts");
  checkError();
  m_Query.exec("delete from m.coverage");
  checkError();
  m_Query.exec("delete from m.polygons");
  checkError();

  m_Query.prepare("insert into m.charts select "
                  "c.id, s.scale, c.swx, c.swy, c.nex, c.ney, c.path from "
                  "main.charts c "
                  "join main.scales s on c.scale_id = s.id "
                  "where s.chartset_id = ?");
  checkError();

  m_Query.bindValue(0, chartset_id);
  m_Query.exec();
  checkError();

  m_Query.prepare("insert into m.coverage select "
                  "v.id, v.type_id, v.chart_id from "
                  "main.charts c "
                  "join main.scales s on c.scale_id = s.id "
                  "join main.coverage v on v.chart_id = c.id "
                  "where s.chartset_id = ?");
  checkError();

  m_Query.bindValue(0, chartset_id);
  m_Query.exec();
  checkError();

  m_Query.prepare("insert into m.polygons select "
                  "p.id, p.cov_id, p.x, p.y from "
                  "main.charts c "
                  "join main.scales s on c.scale_id = s.id "
                  "join main.coverage v on v.chart_id = c.id "
                  "join main.polygons p on p.cov_id = v.id "
                  "where s.chartset_id = ?");
  checkError();

  m_Query.bindValue(0, chartset_id);
  m_Query.exec();
  checkError();
}
