#pragma once
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>


class ChartDatabase {
public:

  ChartDatabase();
  ~ChartDatabase();

  const QSqlQuery& exec(const QString& sql);
  const QSqlQuery& prepare(const QString& sql);
  void exec(QSqlQuery& query);
  bool transaction();
  bool commit();
  void close();

  void loadCharts(int chartset);

private:

  void checkError() const;

  QSqlDatabase m_DB;
  QSqlQuery m_Query;
};

