#pragma once

#include <QObject>
#include "chartdatabase.h"
#include <QMap>
#include "s57chartoutline.h"

class ChartFileReader;
class ChartFileReaderFactory;
class QFileSystemWatcher;

class Updater: public QObject {

  Q_OBJECT

public:

  Updater(QObject* parent = nullptr);
  ~Updater();

public slots:

  void sync();

signals:

  void ready();


private:

  using FactoryMap = QMap<QString, ChartFileReaderFactory*>;
  using ReaderMap = QMap<QString, ChartFileReader*>;
  using ScaleMap = QMap<quint32, quint32>; // scale -> scale_id

  void checkChartDirs();
  void checkChartsDir(const QString& path, bool nofify);
  void loadPlugins();

  void insert(const QString& path, const S57ChartOutline& ch, const ScaleMap& scales);
  void update(const QVariant& id, const S57ChartOutline& ch);

  void insertCov(quint32 chart_id, quint32 type_id, const S57ChartOutline::Region& r);

  ChartDatabase m_db;


  ReaderMap m_readers;
  FactoryMap m_factories;
  QFileSystemWatcher* m_watcher;
  QStringList m_chartDirs;
  bool m_idle;
};
