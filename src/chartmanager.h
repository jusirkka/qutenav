/* -*- coding: utf-8-unix -*-
 *
 * File: src/chartmanager.h
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

#include <QObject>
#include "types.h"
#include "geoprojection.h"
#include <QRectF>
#include <QMap>
#include <QStack>
#include "chartdatabase.h"
#include "chartcover.h"
#include <QCache>
#include "chartupdater.h"
#include "chartproxy.h"

class Camera;
class S57Chart;
class S57ChartOutline;
class QOpenGLContext;
class ChartFileReader;
class ChartFileReaderFactory;
class UpdaterInterface;
class QPainter;
class QThread;



class ChartManager: public QObject {

  Q_OBJECT

public:

  using ChartVector = QVector<S57Chart*>;
  using ChartReaderVector = QVector<ChartFileReader*>;

  static ChartManager* instance();

  QStringList chartSets() const;
  void setChartSet(const QString& charts, const GeoProjection* vproj, bool force = false);
  QString chartSet() const;

  void paintIcon(PickIconData& icon, quint32 chartId, quint32 objectIndex) const;

  const ChartVector& charts() const {return m_charts;}
  const WGS84Polygon& outlines() const {return m_outlines;}
  const ChartReaderVector& readers() const {return m_readers;}
  quint32 nextScale() const {return m_nextScale;}

  GL::ChartProxyQueue& createProxyQueue() {return m_createProxyQueue;}
  GL::ChartProxyQueue& updateProxyQueue() {return m_updateProxyQueue;}
  GL::ChartProxyQueue& destroyProxyQueue() {return m_destroyProxyQueue;}

  void createThreads();

  // flags for updateCharts
  static const quint32 Force = 1;
  static const quint32 UpdateLookups = 2;


  ~ChartManager();

signals:

  void idle();
  void active(const QRectF& viewArea);
  void chartsUpdated(const QRectF& viewArea);
  void updatingCharts();
  void infoResponse(const QString& objectId, const QString& info);
  void infoResponseFull(const S57::InfoType& info);
  void chartSetsUpdated();
  void proxyChanged();
  void chartIndicatorsChanged(const WGS84Polygon& indicators);
  void zoomBottomHit(quint32 scale);

public slots:

  void updateCharts(const Camera* cam, quint32 flags = 0);
  void requestInfo(const WGS84Point& p, bool full);

private slots:

  void manageThreads(S57Chart* chart);
  void manageInfoResponse(const S57::InfoType& info, quint32 tid);
  void updateChartSets(bool clearCache);

private:

  using ChartDataStack = QStack<ChartData>;
  using UpdaterVector = QVector<ChartUpdater*>;
  using ThreadVector = QVector<QThread*>;
  using IDStack = QStack<quint32>;
  using IDList = QList<quint32>;
  using IDVector = QVector<quint32>;
  using IDMap = QMap<quint32, int>;
  using ScaleVector = QVector<quint32>;

  void loadPlugins();
  const ChartCover* getCover(quint32 chart_id,
                             const WGS84Point& sw,
                             const WGS84Point& ne,
                             const GeoProjection* p,
                             quint32 scale);
  void handleSmallScales(const ScaleVector& scales,
                         const GeoProjection* proj,
                         bool noCharts = false);

  KV::RegionMap findCharts(KV::Region& remainingArea, qreal& cov, const ScaleVector& scales, const Camera* cam);
  void createBackground(KV::RegionMap& regions, const GeoProjection* gp, const KV::Region& remaining) const;
  void resetCharts();


  static const inline float viewportFactor = 1.6;
  static const inline float marginFactor = 1.08;
  static const inline float maxScaleRatio = 25;
  static const inline float maxScale = 8000001;
  static const inline float minCoverage = .98;
  static const inline quint32 bgChartLimit = 15;

  ChartManager(QObject *parent = nullptr);
  ChartManager(const ChartManager&) = delete;
  ChartManager& operator=(const ChartManager&) = delete;

  ChartVector m_charts;
  WGS84Polygon m_outlines;
  ChartDatabase m_db;
  WGS84Point m_ref;
  QRectF m_viewport;
  QRectF m_viewArea;
  quint32 m_scale = 0;
  IDMap m_chartIds;
  ScaleVector m_scales;

  UpdaterVector m_workers;
  ThreadVector m_threads;
  IDStack m_idleStack;
  ChartDataStack m_pendingStack;

  QMap<quint32, quint8> m_transactions;
  quint32 m_transactionCounter;

  using InfoTypeVector = QVector<S57::InfoType>;
  QMap<quint32, InfoTypeVector> m_info;

  QThread* m_cacheThread;
  ChartUpdater* m_cacheWorker;
  ChartVector m_cacheQueue;

  GL::ChartProxyQueue m_createProxyQueue;
  GL::ChartProxyQueue m_updateProxyQueue;
  GL::ChartProxyQueue m_destroyProxyQueue;


  using ChartsetNameMap = QMap<QString, int>;
  using FactoryMap = QMap<QString, ChartFileReaderFactory*>;

  ChartsetNameMap m_chartSets;
  ChartReaderVector m_readers;
  ChartFileReader* m_reader;
  FactoryMap m_factories;

  UpdaterInterface* m_updater;


  using CoverCache = QCache<quint32, ChartCover>;

  CoverCache m_coverCache;

  bool m_hadCharts;
  quint32 m_nextScale;

  bool hasBGReader = false;

};

