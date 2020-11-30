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

class ChartUpdater: public QObject {
   Q_OBJECT

public:

  ChartUpdater(quint32 id): QObject(), m_id(id) {}

  quint32 id() const {return m_id;}

public slots:

  void updateChart(S57Chart* chart, const QRectF& viewArea, quint32 scale);
  void createChart(quint32 id, const QString& path, const GeoProjection* p,
                   QRectF viewArea, quint32 scale);

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

  static ChartManager* instance();
  void createThreads(QOpenGLContext* ctx);

  bool isValidScale(const Camera* cam, quint32 scale) const;

  const ChartVector& charts() const {return m_charts;}
  const OutlineVector& outlines() const {return m_outlines;}

  ~ChartManager();

signals:

  void idle();
  void active();
  void chartsUpdated();

public slots:

  void updateCharts(const Camera* cam);

private slots:

  void manageThreads(S57Chart* chart);

private:

  struct ChartData {

    ChartData(S57Chart* c, const QRectF& v, quint32 s)
      : chart(c)
      , viewArea(v)
      , scale(s)
      , id(0)
      , path()
      , proj(nullptr) {}

    ChartData(quint32 i, const QString& pth, GeoProjection* p,
              const QRectF& v, quint32 s)
      : chart(nullptr)
      , viewArea(v)
      , scale(s)
      , id(i)
      , path(pth)
      , proj(p) {}

    S57Chart* chart;
    QRectF viewArea;
    quint32 scale;
    quint32 id;
    QString path;
    GeoProjection* proj;

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

  void updateDB();

  class ChartInfo {
  public:
    int id;
    quint32 scale;
    QRectF bbox;
    WGS84Point ref;
  };

  void createOutline(GeoProjection* p, const ChartInfo& info);

  using ChartInfoVector = QVector<ChartInfo>;
  using IDVector = QVector<quint32>;
  using IDMap = QMap<quint32, quint32>;

  struct LocationArea {
    QRectF bbox;
    ChartInfoVector charts;
  };

  static constexpr float locationRadius = 1000000; // 1000 km
  static constexpr float maxRadius = 15000000; // 15000 km
  static constexpr float viewportFactor = 1.6;
  static constexpr float marginFactor = 1.1;
  static constexpr float maxScaleRatio = 32.;

  using LocationAreaVector = QVector<LocationArea>;

  ChartManager(QObject *parent = nullptr);
  ChartManager(const ChartManager&) = delete;
  ChartManager& operator=(const ChartManager&) = delete;

  ChartVector m_charts;
  OutlineVector m_outlines;
  Database m_db;
  LocationAreaVector m_locationAreas;
  GeoProjection* m_proj;
  QRectF m_viewport;
  quint32 m_scale;
  IDMap m_chartIds;

  UpdaterVector m_workers;
  ThreadVector m_threads;
  IDStack m_idleStack;
  ChartDataStack m_pendingStack;

  bool m_hadCharts;
};

