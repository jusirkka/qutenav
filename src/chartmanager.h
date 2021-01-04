#pragma once

#include <QObject>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <GL/gl.h>
#include "types.h"
#include "geoprojection.h"
#include <QRectF>
#include <QMap>
#include <QStack>

class Camera;
class S57Chart;
class S57ChartOutline;
class QOpenGLContext;

namespace GL {
class Thread;
}

class ChartFileReader;

class ChartUpdater: public QObject {
   Q_OBJECT

public:

  ChartUpdater(quint32 id): QObject(), m_id(id) {}

  quint32 id() const {return m_id;}

public slots:

  void updateChart(S57Chart* chart, quint32 scale,
                   const WGS84Point& sw, const WGS84Point& ne);
  void createChart(quint32 id, const QString& path, quint32 scale,
                   const WGS84Point& sw, const WGS84Point& ne);

signals:

  void done(S57Chart* chart);

private:

  ChartUpdater(const ChartUpdater&) = delete;
  ChartUpdater& operator=(const ChartUpdater&) = delete;

  quint32 m_id;

};


class ChartManager: public QObject {

  Q_OBJECT

public:

  using ChartVector = QVector<S57Chart*>;
  using OutlineVector = QVector<GLfloat>;
  using ChartReaderVector = QVector<ChartFileReader*>;

  static ChartManager* instance();
  void createThreads(QOpenGLContext* ctx);

  bool isValidScale(const Camera* cam, quint32 scale) const;
  QStringList chartSets() const;
  void setChartSet(const QString& charts, const GeoProjection* vproj);

  const ChartVector& charts() const {return m_charts;}
  const OutlineVector& outlines() const {return m_outlines;}
  const ChartReaderVector& readers() const {return m_readers;}

  ~ChartManager();

signals:

  void idle();
  void active();
  void chartsUpdated(const QRectF& viewArea);

public slots:

  void updateCharts(const Camera* cam, bool force = false);

private slots:

  void manageThreads(S57Chart* chart);

private:

  struct ChartData {

    ChartData(S57Chart* c,
              quint32 s, const WGS84Point& sw0, const WGS84Point& ne0)
      : chart(c)
      , id(0)
      , path()
      , scale(s)
      , sw(sw0)
      , ne(ne0)
    {}

    ChartData(quint32 i, const QString& pth,
              quint32 s, const WGS84Point& sw0, const WGS84Point& ne0)
      : chart(nullptr)
      , id(i)
      , path(pth)
      , scale(s)
      , sw(sw0)
      , ne(ne0)
    {}

    S57Chart* chart;
    quint32 id;
    QString path;
    quint32 scale;
    WGS84Point sw;
    WGS84Point ne;

    ChartData() = default;
    ChartData(const ChartData&) = default;
    ChartData& operator= (const ChartData&) = default;
  };

  using ChartDataStack = QStack<ChartData>;
  using IDStack = QStack<quint32>;
  using UpdaterVector = QVector<ChartUpdater*>;
  using ThreadVector = QVector<GL::Thread*>;


  class Database {
  public:

    Database();

    const QSqlQuery& exec(const QString& sql);
    const QSqlQuery& prepare(const QString& sql);
    void exec(QSqlQuery& query);
    bool transaction();
    bool commit();
    void close();

  private:

    QSqlDatabase m_DB;
    QSqlQuery m_Query;
  };

  void fillChartsTable();
  void fillChartsetsTable();

  class ChartInfo {
  public:
    int id;
    quint32 scale;
    QRectF bbox;
    WGS84Point ref;
  };

  void createOutline(const GeoProjection* p, const ChartInfo& info);

  using ChartInfoVector = QVector<ChartInfo>;
  using IDVector = QVector<quint32>;
  using IDMap = QMap<quint32, quint32>;

  struct LocationArea {
    QRectF bbox;
    ChartInfoVector charts;
  };

  static constexpr float locationRadius = 1000000; // 1000 km
  static constexpr float maxRadius = 15000000; // 15000 km
  static constexpr float viewportFactor = 1.9;
  static constexpr float marginFactor = 1.08;
  static constexpr float maxScaleRatio = 32.;

  using LocationAreaVector = QVector<LocationArea>;

  ChartManager(QObject *parent = nullptr);
  ChartManager(const ChartManager&) = delete;
  ChartManager& operator=(const ChartManager&) = delete;

  ChartVector m_charts;
  OutlineVector m_outlines;
  Database m_db;
  LocationAreaVector m_locationAreas;
  WGS84Point m_ref;
  QRectF m_viewport;
  QRectF m_viewArea;
  quint32 m_scale;
  IDMap m_chartIds;

  UpdaterVector m_workers;
  ThreadVector m_threads;
  IDStack m_idleStack;
  ChartDataStack m_pendingStack;

  using ChartsetNameMap = QMap<QString, int>;
  using FilterMap = QMap<QString, QStringList>;

  ChartFileReader* createReader(const QString& name) const;

  ChartsetNameMap m_chartSets;
  ChartReaderVector m_readers;
  ChartFileReader* m_reader;
  const FilterMap m_filters;

  bool m_hadCharts;
};

