#pragma once

#include <QObject>
#include "chartdatabase.h"
#include <QMap>
#include "s57chartoutline.h"

class ChartFileReader;
class ChartFileReaderFactory;

class Updater: public QObject {

  Q_OBJECT

public:

  Updater(QObject* parent = nullptr);
  ~Updater();

public slots:

  void sync();
  QString ping() const;


signals:

  void ready();


private:

  using IdSet = QSet<quint32>;

  using FactoryMap = QMap<QString, ChartFileReaderFactory*>;
  using ReaderMap = QMap<QString, ChartFileReader*>;
  using ScaleMap = QMap<quint32, quint32>; // scale -> scale_id

  void loadPlugins();

  void checkChartDirs();

  void checkChartsDir(const QString& path, IdSet& prevCharts);
  void deleteCharts(const IdSet& ids);
  void deleteFrom(const QString& chartName, const IdSet& ids);
  void insert(const QString& path, const S57ChartOutline& ch, const ScaleMap& scales);
  void update(const QVariant& id, const S57ChartOutline& ch);
  void insertCov(quint32 chart_id, quint32 type_id, const S57ChartOutline::Region& r);
  void cleanupDB();

  ChartDatabase m_db;


  ReaderMap m_readers;
  FactoryMap m_factories;
  QStringList m_chartDirs;
};
