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

class Camera;
class S57Chart;
class S57ChartOutline;
class QOpenGLContext;
class ChartFileReader;
class ChartFileReaderFactory;
class UpdaterInterface;
class QPainter;

namespace GL {
class Thread;
}



class ChartManager: public QObject {

  Q_OBJECT

public:

  using ChartVector = QVector<S57Chart*>;
  using ChartReaderVector = QVector<ChartFileReader*>;

  static ChartManager* instance();
  void createThreads(QOpenGLContext* ctx);

  QStringList chartSets() const;
  void setChartSet(const QString& charts, const GeoProjection* vproj, bool force = false);
  QString chartSet() const;

  void paintIcon(QPainter& painter, quint32 chartId, quint32 objectIndex) const;

  const ChartVector& charts() const {return m_charts;}
  const GL::VertexVector& outlines() const {return m_outlines;}
  const ChartReaderVector& readers() const {return m_readers;}

  // flags for updateCharts
  static const quint32 Force = 1;
  static const quint32 UpdateLookups = 2;


  ~ChartManager();

signals:

  void idle();
  void active();
  void chartsUpdated(const QRectF& viewArea);
  void infoResponse(const QString& objectId, const QString& info);
  void chartSetsUpdated();

public slots:

  void updateCharts(const Camera* cam, quint32 flags = 0);
  void requestInfo(const WGS84Point& p);

private slots:

  void manageThreads(S57Chart* chart);
  void manageInfoResponse(const S57::InfoType& info, quint32 tid);
  void updateChartSets();

private:

  using ChartDataStack = QStack<ChartData>;
  using IDStack = QStack<quint32>;
  using UpdaterVector = QVector<ChartUpdater*>;
  using ThreadVector = QVector<GL::Thread*>;

  void createOutline(const WGS84Point& sw, const WGS84Point& ne);
  void loadPlugins();
  const ChartCover* getCover(quint32 chart_id,
                             const WGS84Point& sw,
                             const WGS84Point& ne,
                             const GeoProjection* p);


  using IDVector = QVector<quint32>;
  using IDMap = QMap<quint32, quint32>;
  using ScaleVector = QVector<quint32>;

  static constexpr float viewportFactor = 1.65;
  static constexpr float marginFactor = 1.08;
  static constexpr float maxScaleRatio = 32;
  static constexpr float maxScale = 25000000;
  static constexpr float minCoverage = .975;

  ChartManager(QObject *parent = nullptr);
  ChartManager(const ChartManager&) = delete;
  ChartManager& operator=(const ChartManager&) = delete;

  ChartVector m_charts;
  GL::VertexVector m_outlines;
  ChartDatabase m_db;
  WGS84Point m_ref;
  QRectF m_viewport;
  QRectF m_viewArea;
  quint32 m_scale;
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

  GL::Thread* m_cacheThread;
  ChartUpdater* m_cacheWorker;
  ChartVector m_cacheQueue;


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

};

