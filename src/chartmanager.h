#pragma once

#include <QObject>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <GL/gl.h>
#include "types.h"
#include "geoprojection.h"
#include <QRectF>
#include <QMap>
#include <QThread>

class Camera;
class S57Chart;
class S57ChartOutline;

class ChartUpdater: public QThread {
   Q_OBJECT
public:

  ChartUpdater(S57Chart* chart, const QRectF& viewArea, quint32 scale);
  ChartUpdater(quint32 id, const QString& path, const GeoProjection* p, const QRectF& viewArea, quint32 scale);

  S57Chart* chart() {return m_chart;}

private:

  ChartUpdater() = delete;
  ChartUpdater(const ChartUpdater&) = delete;
  ChartUpdater& operator=(const ChartUpdater&) = delete;

  void run() override;

  S57Chart* m_chart;
  quint32 m_id;
  QString m_path;
  const GeoProjection* m_proj;
  QRectF m_area;
  quint32 m_scale;
};


class ChartManager: public QObject {

  Q_OBJECT

public:

  using ChartVector = QVector<S57Chart*>;
  using OutlineVector = QVector<GLfloat>;

  static ChartManager* instance();


  bool isValidScale(const Camera* cam, quint32 scale) const;


  const ChartVector& charts() const {return m_charts;}
  const OutlineVector& outlines() const {return m_outlines;}

signals:

  void idle();
  void active();
  void chartsUpdated();
  void objectsUpdated();

public slots:

  void updateCharts(const Camera* cam);

private slots:

  void manageThreads();

private:

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
  using UpdaterMap = QMap<quint32, ChartUpdater*>;

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
  IDMap m_ids;

  bool m_hadCharts;
  bool m_onlyObjects;
  UpdaterMap m_threads;
};

