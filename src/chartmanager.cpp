/* -*- coding: utf-8-unix -*-
 *
 * File: src/chartmanager.cpp
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
#include "chartmanager.h"
#include <QFile>
#include <QDir>
#include <QTextStream>
#include <QThread>
#include "logging.h"
#include "s57chart.h"
#include <QStandardPaths>
#include <QVariant>
#include <QDirIterator>
#include "camera.h"
#include <QOpenGLContext>
#include "chartfilereader.h"
#include <QDate>
#include <QScopedPointer>
#include <QRegion>
#include <QPluginLoader>
#include <QLibraryInfo>
#include "cachereader.h"
#include "dbupdater_interface.h"
#include "gnuplot.h"
#include "conf_mainwindow.h"
#include "conf_quick.h"
#include "platform.h"
#include "utils.h"
#include "slotcounter.h"

ChartManager* ChartManager::instance() {
  static ChartManager* m = new ChartManager();
  return m;
}

ChartManager::ChartManager(QObject *parent)
  : QObject(parent)
  , m_db()
  , m_transactionCounter(0)
  , m_reader(nullptr)
  , m_updater(new UpdaterInterface(this))
  , m_coverCache(1000 * sizeof(ChartCover))
  , m_nextScale(0)
{

  loadPlugins();
  m_readers.append(new CacheReader);

  // append background chart generator
  if (m_factories.contains("gshhs")) {
    auto reader = m_factories["gshhs"]->loadReader(QStringList {"gshhg"});
    if (reader != nullptr) {
      m_readers.append(reader);
    }
  }

  // create readers & chartsets
  updateChartSets(false);

  connect(m_updater, &UpdaterInterface::ready, this, &ChartManager::updateChartSets);
}

void ChartManager::updateChartSets(bool clearCache) {

  qCDebug(CMGR) << "Checking cache";
  checkCache(clearCache);

  auto prevSets = m_chartSets.keys();

  QSqlQuery r = m_db.exec("select name from chartsets");
  while (r.next()) {
    const QString name = r.value(0).toString();

    if (!m_factories.contains(name)) continue;

    if (m_chartSets.contains(m_factories[name]->displayName())) {
      prevSets.removeOne(m_factories[name]->displayName());
    } else {
      auto it = std::find_if(m_readers.cbegin(), m_readers.cend(), [name] (const ChartFileReader* r) {
        return r->name() == name;
      });
      auto index = std::distance(m_readers.cbegin(), it);

      if (index < m_readers.size()) { // already created
        m_chartSets[m_factories[name]->displayName()] = index;
      } else {
        auto reader = m_factories[name]->loadReader(Conf::MainWindow::ChartFolders());
        if (reader == nullptr) continue;
        m_chartSets[m_factories[name]->displayName()] = m_readers.size();
        m_readers.append(reader);
      }
    }
  }

  for (auto dname: prevSets) {
    if (m_readers[m_chartSets[dname]] == m_reader) {
      m_reader = m_readers.first();
    }
    m_chartSets.remove(dname);
  }

  qCDebug(CMGR) << "chartsets" << m_chartSets.keys();
  if (m_reader != nullptr) qCDebug(CMGR) << "selected set" << m_reader->name();

  emit chartSetsUpdated();
}

void ChartManager::loadPlugins() {
  const auto& staticFactories = QPluginLoader::staticInstances();
  for (auto plugin: staticFactories) {
    auto factory = qobject_cast<ChartFileReaderFactory*>(plugin);
    if (!factory) continue;
    qCDebug(CMGR) << "Loaded reader plugin" << factory->name();
    m_factories[factory->name()] = factory;
  }

  const QString loc = baseAppName();
  const QString base = QLibraryInfo::location(QLibraryInfo::PluginsPath);
  QDir pluginsDir(QString("%1/%2").arg(base).arg(loc));
  qCDebug(CMGR) << "Searching reader plugins in" << pluginsDir.dirName();

  const QStringList plugins = pluginsDir.entryList(QStringList(),
                                                   QDir::Files | QDir::Readable);
  for (auto& plugin: plugins) {
    QPluginLoader loader(pluginsDir.absoluteFilePath(plugin));
    auto factory = qobject_cast<ChartFileReaderFactory*>(loader.instance());
    if (!factory) continue;
    qCDebug(CMGR) << "Loaded reader plugin" << factory->name();
    m_factories[factory->name()] = factory;
  }
}


QStringList ChartManager::chartSets() const {
  return m_chartSets.keys();
}

QString ChartManager::chartSet() const {
  for (auto it = m_chartSets.cbegin(); it != m_chartSets.cend(); ++it) {
    if (m_reader == m_readers[it.value()]) {
      return it.key();
    }
  }
  return "None";
}

void ChartManager::setChartSet(const QString &name, const GeoProjection* vproj, bool force) {

  if (!m_chartSets.contains(name)) {
    qCWarning(CMGR) << "Unknown chartset" << name;
    return;
  }
  if (!force && m_reader == m_readers[m_chartSets[name]]) {
    qCDebug(CMGR) << "Current chartset is already" << name;
    return;
  }

  qCDebug(CMGR) << "Changing chartset" << name;

  m_reader = m_readers[m_chartSets[name]];
  m_outlines.clear();
  m_scales.clear();

  m_ref = vproj->reference();

  // get chartset id
  QSqlQuery r0 = m_db.prepare("select id "
                              "from main.chartsets "
                              "where name=?");
  r0.bindValue(0, QVariant::fromValue(m_reader->name()));
  m_db.exec(r0);
  r0.first();
  const uint set_id = r0.value(0).toUInt();

  // get available scales
  QSqlQuery r1 = m_db.prepare("select scale, priority "
                              "from main.scales "
                              "where chartset_id=? order by priority");
  r1.bindValue(0, set_id);
  m_db.exec(r1);
  IDVector priorities;
  while (r1.next()) {
    m_scales.append(r1.value(0).toUInt());
    priorities.append(r1.value(1).toUInt());
  }
  std::sort(m_scales.begin(), m_scales.end());

  // load charts to attached memory db
  m_db.loadCharts(set_id);

  // outlines
  SlotCounter counter;
  IDVector ids;
  for (const auto prio: priorities) {

    if (counter.full()) break;

    // qDebug() << "Checking priority =" << prio << "charts";

    QSqlQuery t0 = m_db.prepare("select chart_id, swx, swy, nex, ney "
                                "from m.charts "
                                "where priority = ?");
    t0.bindValue(0, prio);
    m_db.exec(t0);
    while (t0.next()) {
      const auto sw = WGS84Point::fromLL(t0.value(1).toDouble(), t0.value(2).toDouble());
      const auto ne = WGS84Point::fromLL(t0.value(3).toDouble(), t0.value(4).toDouble());
      counter.fill(sw, ne, prio);
      if (counter.updated()) {
        const auto id = t0.value(0).toUInt();
        // qDebug() << "Adding chart" << id;
        ids.append(id);
      }
    }
  }

  const auto sql = QString("select cv.id, p.x, p.y from "
                           "m.polygons p join m.coverage cv "
                           "on p.cov_id = cv.id "
                           "where cv.type_id=1 and cv.chart_id in (?%1) "
                           "order by cv.id, p.id")
      .arg(QString(",?").repeated(ids.size() - 1));

  QSqlQuery r2 = m_db.prepare(sql);
  int cnt = 0;
  for (const auto id: ids) {
    r2.bindValue(cnt, id);
    cnt++;
  }
  m_db.exec(r2);

  int prev = -1;
  WGS84PointVector ps;
  while (r2.next()) {
    const int cid = r2.value(0).toInt();
    if (cid != prev) {
      if (!ps.isEmpty()) {
        m_outlines << ps;
      }
      ps.clear();
    }
    ps << WGS84Point::fromLL(r2.value(1).toDouble(), r2.value(2).toDouble());
    prev = cid;
  }
  if (!ps.isEmpty()) {
    m_outlines << ps;
  }
}

void ChartManager::createThreads() {
  const int numThreads = numberOfChartThreads();
  qCDebug(CMGR) << "number of chart updaters =" << numThreads;
  for (int i = 0; i < numThreads; ++i) {
    qCDebug(CMGR) << "creating thread" << i;
    auto thread = new QThread;
    qCDebug(CMGR) << "creating worker" << i;
    auto worker = new ChartUpdater(m_workers.size());
    m_idleStack.push(worker->id());
    connect(thread, &QThread::finished, worker, &QObject::deleteLater);
    connect(worker, &ChartUpdater::done, this, &ChartManager::manageThreads);
    connect(worker, &ChartUpdater::infoResponse, this, &ChartManager::manageInfoResponse);
    qCDebug(CMGR) << "moving worker to thread" << i;
    worker->moveToThread(thread);
    qCDebug(CMGR) << "starting thread" << i;
    thread->start();
    m_threads.append(thread);
    m_workers.append(worker);
  }

  m_cacheThread = new QThread;
  m_cacheWorker = new ChartUpdater(m_workers.size());
  m_cacheWorker->moveToThread(m_cacheThread);
  connect(m_cacheThread, &QThread::finished, m_cacheWorker, &QObject::deleteLater);
  m_cacheThread->start();
  qCDebug(CMGR) << "threads started";
}

ChartManager::~ChartManager() {
  for (QThread* thread: m_threads) {
    thread->quit();
    thread->wait();
  }
  qDeleteAll(m_threads);

  // Cache charts before quitting
  for (auto chart: m_charts) {
    qCDebug(CMGR) << "Caching" << chart->id() << "before exit";
    QMetaObject::invokeMethod(m_cacheWorker, "cacheChart",
                              Q_ARG(S57Chart*, chart));
  }

  m_cacheThread->quit();
  m_cacheThread->wait();
  delete m_cacheThread;

  qDeleteAll(m_readers);
}



const ChartCover* ChartManager::getCover(quint32 chart_id,
                                         const WGS84Point &sw,
                                         const WGS84Point &ne,
                                         const GeoProjection *p,
                                         quint32 scale) {
  if (!m_coverCache.contains(chart_id)) {
    QSqlQuery r = m_db.prepare("select c.id, c.type_id, p.x, p.y "
                               "from m.polygons p "
                               "join m.coverage c on p.cov_id = c.id "
                               "where c.chart_id=? order by c.id, p.id");
    r.bindValue(0, chart_id);
    m_db.exec(r);
    int prev = -1;
    int type_id = -1;
    WGS84Polygon cov;
    WGS84Polygon nocov;
    WGS84PointVector ps;
    while (r.next()) {
      const int cid = r.value(0).toInt();
      if (cid != prev) {
        if (type_id == 1) {
          cov << ps;
        } else if (type_id == 2) {
          nocov << ps;
        }
        ps.clear();
      }
      ps << WGS84Point::fromLL(r.value(2).toDouble(), r.value(3).toDouble());
      prev = cid;
      type_id = r.value(1).toInt();
    }
    if (!ps.isEmpty()) {
      if (type_id == 1) {
        cov << ps;
      } else if (type_id == 2) {
        nocov << ps;
      }
    }

    auto c = new ChartCover(cov, nocov, sw, ne, p, scale);
    // chartcover::tognuplot(cov, nocov, c->region(p), sw, ne, p, chart_id);
    m_coverCache.insert(chart_id, c);
  }

  return m_coverCache[chart_id];
}

void ChartManager::updateCharts(const Camera *cam, quint32 flags) {

  // qCDebug(CMGR) << "updateCharts" << m_idleStack.size() << "/" << m_workers.size();

  if (m_idleStack.size() != m_workers.size()) return;

  m_viewport = m_viewport.translated(cam->geoprojection()->fromWGS84(m_ref));
  m_ref = cam->eye();

  const QRectF vp = cam->boundingBox();
  // qCDebug(CMGR) << "ChartManager::updateCharts" << vp;
  if (!vp.isValid()) return;

  if (m_viewport.contains(vp) && cam->scale() == m_scale && flags == 0) return;

  // Inform chartdisplay that we are busy now
  emit updatingCharts();

  // setup viewarea
  qreal mw = vp.width() * (viewportFactor - 1) / 2;
  qreal mh = vp.height() * (viewportFactor - 1) / 2;
  m_viewport = vp + QMarginsF(mw, mh, mw, mh);

  mw = m_viewport.width() * (marginFactor - 1) / 2;
  mh = m_viewport.height() * (marginFactor - 1) / 2;
  m_viewArea = m_viewport + QMarginsF(mw, mh, mw, mh);

  const qreal a0 = m_viewArea.width() / m_viewArea.height();
  if (cam->aspect() >  a0) {
    m_viewArea.setWidth(cam->aspect() * m_viewArea.height());
  } else {
    m_viewArea.setHeight(m_viewArea.width() / cam->aspect());
  }
  m_viewArea.moveCenter(QPointF(0., 0.));

  // sort available scales
  m_scale = cam->scale();
  ScaleVector scaleCandidates;
  ScaleVector smallScales;
  if (!m_scales.isEmpty() && m_scale * maxScaleRatio <= m_scales.first()) {
    scaleCandidates << m_scales.first();
  } else if (m_scale < maxScale) {
    for (quint32 sc: m_scales) {
      if (sc > maxScaleRatio * m_scale) continue;
      if (m_scale > maxScaleRatio * sc) continue;
      scaleCandidates << sc;
    }
    // select largest scale less or equal than target scale as preferred scale
    int index = scaleCandidates.size() - 1;
    for (int i = 0; i < scaleCandidates.size(); i++) {
      if (scaleCandidates[i] > m_scale) {
        index = i - 1;
        break;
      }
    }
    while (index > 0) {
      smallScales.append(scaleCandidates.takeFirst());
      index--;
    }
  }

  qCDebug(CMGR) << "target" << m_scale << ", candidates" << scaleCandidates;



  KV::Region remainingArea(m_viewArea);
  qreal cov = 0.;
  KV::RegionMap regions = findCharts(remainingArea, cov, scaleCandidates, cam);

  if (regions.isEmpty()) {
    std::reverse(smallScales.begin(), smallScales.end());
    qCDebug(CMGR) << "target" << m_scale << ", candidates" << smallScales;
    while (!smallScales.isEmpty()) {
      regions = findCharts(remainingArea, cov, ScaleVector {smallScales.takeFirst()}, cam);
      if (!regions.isEmpty()) break;
    }
    std::reverse(smallScales.begin(), smallScales.end());
  }

  if (cov < minCoverage && !regions.isEmpty() && !remainingArea.isEmpty()) {
    createBackground(regions, cam->geoprojection(), remainingArea);
  }

  // Signal indicators of small scale charts
  handleSmallScales(smallScales, cam->geoprojection());

  IDVector newCharts;
  IDVector bgCharts;
  for (auto id: regions.keys()) {
    if (!m_chartIds.contains(id)) {
      if (ChartFileReader::isBGIndex(id)) {
        bgCharts.append(id);
      } else {
        newCharts.append(id);
      }
    } else {
      m_chartIds.remove(id);
    }
  }

  m_hadCharts = !m_charts.isEmpty();

  // queue old charts for caching
  for (auto it = m_chartIds.constBegin(); it != m_chartIds.constEnd(); ++it) {
    m_cacheQueue.append(m_charts[it.value()]);
    m_charts[it.value()] = nullptr;
  }
  ChartVector charts;
  // update m_chartIds
  m_chartIds.clear();
  for (S57Chart* c: m_charts) {
    if (c != nullptr) {
      m_chartIds[c->id()] = charts.size();
      charts.append(c);
    }
  }
  m_charts = charts;

  const bool noCharts = m_charts.isEmpty() && newCharts.isEmpty();
  // create pending chart update data
  for (S57Chart* c: m_charts) {
    m_pendingStack.push(ChartData(c, m_scale, regions[c->id()].toWGS84(cam->geoprojection()),
                        (flags & UpdateLookups) != 0));
  }
  // create pending chart creation data
  if (!newCharts.isEmpty()) {
    QString sql = "select chart_id, priority, path "
                  "from m.charts "
                  "where chart_id in (";
    sql += QString("?,").repeated(newCharts.size());
    sql = sql.replace(sql.length() - 1, 1, ")");
    QSqlQuery r = m_db.prepare(sql);
    for (int i = 0; i < newCharts.size(); i++) {
      r.bindValue(i, QVariant::fromValue(newCharts[i]));
    }
    m_db.exec(r);
    while (r.next()) {
      const quint32 id = r.value(0).toUInt();
      const int prio = r.value(1).toInt();
      const auto path = r.value(2).toString();
      qCDebug(CMGR) << "New chart" << path << "priority" << prio;
      m_pendingStack.push(ChartData(id, prio, path, m_scale,
                                    regions[id].toWGS84(cam->geoprojection())));
    }
  }
  // create pending bg chart creation data
  for (const auto id: bgCharts) {
    m_pendingStack.push(ChartData(id, 0, QString("gshhs://production/%1").arg(id), m_scale,
                                  regions[id].toWGS84(cam->geoprojection())));
  }

  while (!m_pendingStack.isEmpty() && !m_idleStack.isEmpty()) {
    const ChartData d = m_pendingStack.pop();
    const quint32 index = m_idleStack.pop();
    ChartUpdater* dest = m_workers[index];
    if (d.chart != nullptr) {
      QMetaObject::invokeMethod(dest, "updateChart",
                                Q_ARG(const ChartData&, d));
    } else {
      QMetaObject::invokeMethod(dest, "createChart",
                                Q_ARG(const ChartData&, d));
    }
  }

  if (noCharts) {
    // qCDebug(CMGR) << "ChartManager::updateCharts: idle";
    emit idle();
  }
}

KV::RegionMap ChartManager::findCharts(KV::Region& remainingArea, qreal& cov, const ScaleVector& scales, const Camera *cam) {

  KV::RegionMap regions;

  cov = 0.;

  const auto totarea = remainingArea.area();


  for (quint32 scale: scales) {

    const auto box = remainingArea.boundingRect();
    const WGS84Point sw0 = cam->geoprojection()->toWGS84(box.topLeft()); // inverted y-axis
    const WGS84Point ne0 = cam->geoprojection()->toWGS84(box.bottomRight()); // inverted y-axis

    // select charts
    QSqlQuery r = m_db.prepare("select chart_id, swx, swy, nex, ney, path "
                               "from m.charts "
                               "where scale = ? and "
                               "      swx < ? and swy < ? and "
                               "      nex > ? and ney > ?");
    r.bindValue(0, scale);
    r.bindValue(1, ne0.lng(sw0));
    r.bindValue(2, ne0.lat());
    r.bindValue(3, sw0.lng());
    r.bindValue(4, sw0.lat());

    m_db.exec(r);
    while (r.next() && cov < minCoverage) {
      quint32 id = r.value(0).toUInt();

      auto sw = WGS84Point::fromLL(r.value(1).toDouble(), r.value(2).toDouble());
      auto ne = WGS84Point::fromLL(r.value(3).toDouble(), r.value(4).toDouble());
      auto c = getCover(id, sw, ne, cam->geoprojection(), scale);

      qCDebug(CMGR) << "Candidate" << id << scale;
      const auto region = c->region(cam->geoprojection()).intersected(m_viewArea);
      if (region.isEmpty()) continue;

      auto delta = remainingArea.intersected(region);
      if (delta.area() / totarea < 1.e-3) continue;

      regions[id] = delta;

      remainingArea -= delta;
      cov = 1 - remainingArea.area() / totarea;
      qCDebug(CMGR) << "covers" << region.area() / totarea * 100
                    << ", subtracts" << delta.area() / totarea * 100
                    << ", remaining" << (1 - cov) * 100;
    }
    if (cov >= minCoverage) break;
  }

  return regions;
}


void ChartManager::manageThreads(S57Chart* chart) {

  if (chart != nullptr) {
    auto chartId = chart->id();
    if (!m_chartIds.contains(chartId)) {
      m_chartIds[chartId] = m_charts.size();
      m_charts.append(chart);
      m_createProxyQueue.append(chart->proxy());
      connect(chart, &S57Chart::destroyProxy, this, [this] (GL::ChartProxy* proxy) {
        m_destroyProxyQueue.append(proxy);
        emit proxyChanged();
      });
    }
    m_updateProxyQueue.append(chart->proxy());
    emit proxyChanged();
  }

  auto dest = qobject_cast<ChartUpdater*>(sender());
  if (!m_pendingStack.isEmpty()) {
    const ChartData d = m_pendingStack.pop();
    if (d.chart != nullptr) {
      QMetaObject::invokeMethod(dest, "updateChart",
                                Q_ARG(const ChartData&, d));

    } else {
      QMetaObject::invokeMethod(dest, "createChart",
                                Q_ARG(const ChartData&, d));
    }
  } else {
    m_idleStack.push(dest->id());
    while (!m_cacheQueue.isEmpty()) {
      auto chart = m_cacheQueue.takeFirst();
      QMetaObject::invokeMethod(m_cacheWorker, "cacheChart",
                                Q_ARG(S57Chart*, chart));
    }
  }


  // qCDebug(CMGR) << "chartmanager" << m_idleStack.size() << "/" << m_workers.size();
  if (m_idleStack.size() == m_workers.size()) {
    // Sort charts in priority order
    std::sort(m_charts.begin(), m_charts.end(), [] (const S57Chart* c1, const S57Chart* c2) {
      return c1->priority() > c2->priority();
    });
    for (int i = 0; i < m_charts.size(); ++i) {
      m_chartIds[m_charts[i]->id()] = i;
    }
    if (!m_hadCharts) {
      // qCDebug(CMGR) << "chartmanager: manageThreads: active";
      emit active(m_viewArea);
    } else {
      // qCDebug(CMGR) << "chartmanager: manageThreads: charts updated";
      emit chartsUpdated(m_viewArea);
    }
  }
}

void ChartManager::handleSmallScales(const ScaleVector& scales,
                                     const GeoProjection* proj) {

  WGS84Polygon indicators;
  m_nextScale = 0;

  if (!Conf::Quick::IndicateScales() || scales.isEmpty()) {
    emit chartIndicatorsChanged(indicators);
    return;
  }

  const WGS84Point sw0 = proj->toWGS84(m_viewArea.topLeft()); // inverted y-axis
  const WGS84Point ne0 = proj->toWGS84(m_viewArea.bottomRight()); // inverted y-axis

  // find m_nextScale
  const auto sql0 = QString("select max(scale) "
                            "from m.charts "
                            "where scale in (?%1) and "
                            "swx < ? and swy < ? and nex > ? and ney > ?")
      .arg(QString(",?").repeated(scales.size() - 1));

  QSqlQuery r0 = m_db.prepare(sql0);

  int cnt = 0;
  for (auto s: scales) r0.bindValue(cnt++, s);
  r0.bindValue(cnt++, ne0.lng(sw0));
  r0.bindValue(cnt++, ne0.lat());
  r0.bindValue(cnt++, sw0.lng());
  r0.bindValue(cnt++, sw0.lat());

  m_db.exec(r0);

  if (r0.first()) {
    m_nextScale = r0.value(0).toUInt();
  } else {
    emit chartIndicatorsChanged(indicators);
    return;
  }

  const auto sql1 = QString("select cv.id, p.x, p.y from "
                            "m.polygons p "
                            "join m.coverage cv on p.cov_id = cv.id "
                            "join m.charts c on c.chart_id = cv.chart_id "
                            "where cv.type_id=1 and c.scale in (?%1) and "
                            "c.swx < ? and c.swy < ? and c.nex > ? and c.ney > ? "
                            "order by cv.id, p.id")
      .arg(QString(",?").repeated(scales.size() - 1));

  QSqlQuery r1 = m_db.prepare(sql1);

  cnt = 0;
  for (auto s: scales) r1.bindValue(cnt++, s);
  r1.bindValue(cnt++, ne0.lng(sw0));
  r1.bindValue(cnt++, ne0.lat());
  r1.bindValue(cnt++, sw0.lng());
  r1.bindValue(cnt++, sw0.lat());

  m_db.exec(r1);

  int prev = -1;
  WGS84PointVector ps;
  while (r1.next()) {
    const int cid = r1.value(0).toInt();
    if (cid != prev) {
      if (!ps.isEmpty()) {
        indicators << ps;
      }
      ps.clear();
    }
    ps << WGS84Point::fromLL(r1.value(1).toDouble(), r1.value(2).toDouble());
    prev = cid;
  }
  if (!ps.isEmpty()) {
    indicators << ps;
  }

  emit chartIndicatorsChanged(indicators);
}

void ChartManager::createBackground(KV::RegionMap& regions,
                                    const GeoProjection* gp,
                                    const KV::Region& c) const {
  const auto r = c.boundingRect();
  const auto sw = gp->toWGS84(r.topLeft());
  const auto ne = gp->toWGS84(r.bottomRight());

  const auto levels = ChartFileReader::bgLevels();

  quint32 level = 0;
  for (const auto lvl: levels) {
    level = lvl;
    const auto nbg = ChartFileReader::numBGIndices(sw, ne, lvl);
    if (nbg < bgChartLimit) break;
  }

  const auto bgIndices = ChartFileReader::bgIndices(sw, ne, level);

  KV::Region remainingArea = c;

  const auto totarea = remainingArea.area();
  qreal cov = 0.;

  const qreal d = .1 * level;
  for (auto it = bgIndices.cbegin(); it != bgIndices.cend(); ++it) {
    const QRectF box(gp->fromWGS84(WGS84Point::fromLL(it.value().lng() - d, it.value().lat() - d)),
                     gp->fromWGS84(WGS84Point::fromLL(it.value().lng() + d, it.value().lat() + d)));

    auto delta = remainingArea.intersected(box);
    if (delta.area() / totarea < 1.e-3) continue;

    regions[it.key()] = delta;

    remainingArea -= delta;
    cov = 1 - remainingArea.area() / totarea;
    qCDebug(CMGR) << it.value().print(WGS84Point::Units::Deg, 0)
                  << ": subtracts" << delta.area() / totarea * 100
                  << ", remaining" << (1 - cov) * 100;

    if (cov >= minCoverage) break;
  }
}

void ChartManager::requestInfo(const WGS84Point &p, bool full) {

  if (m_charts.empty()) return;

  const auto sql = QString("select chart_id, swx, swy, nex, ney "
                           "from m.charts "
                           "where chart_id in (?%1)")
      .arg(QString(",?").repeated(m_charts.size() - 1));

  QSqlQuery r = m_db.prepare(sql);
  for (int i = 0; i < m_charts.size(); i++) {
    r.bindValue(i, m_charts[i]->id());
  }

  m_db.exec(r);
  ChartVector reqs;
  while (r.next()) {
    if (p.containedBy(WGS84Point::fromLL(r.value(1).toDouble(), r.value(2).toDouble()),
                      WGS84Point::fromLL(r.value(3).toDouble(), r.value(4).toDouble()))) {
      auto index = m_chartIds[r.value(0).toUInt()];
      reqs.append(m_charts[index]);
    }
  }

  if (reqs.isEmpty()) {
    // qCDebug(CMGR) << p.print() << "is outside of area covered by current chartset";
    emit infoResponse("", "");
    return;
  }

  auto tid = m_transactionCounter;
  m_transactionCounter += 1;
  m_transactions[tid] = m_charts.size() - reqs.size();
  int counter = 0;
  for (auto chart: reqs) {
    QMetaObject::invokeMethod(m_workers[counter], "requestInfo",
                              Q_ARG(S57Chart*, chart),
                              Q_ARG(const WGS84Point&, p),
                              Q_ARG(quint32, m_scale),
                              Q_ARG(bool, full),
                              Q_ARG(quint32, tid));
    counter = (counter + 1) % m_workers.size();
  }
}

void ChartManager::manageInfoResponse(const S57::InfoType& info, quint32 tid) {

  m_transactions[tid] = m_transactions[tid] + 1;

  m_info[tid] << info;

  if (m_transactions[tid] == m_charts.size()) {
    // All responses received
    auto it = std::max_element(m_info[tid].cbegin(), m_info[tid].cend(), [] (const S57::InfoType& t1, const S57::InfoType& t2) {
      if (t1.priority != t2.priority) return t1.priority < t2.priority;
      if (t1.objectId.size() != t2.objectId.size()) return t1.objectId.size() < t2.objectId.size();
      return t1.descriptions.size() < t2.descriptions.size();
    });
    if (it != m_info[tid].cend()) {
      if (!it->descriptions.isEmpty()) {
        emit infoResponseFull(*it);
      } else {
        emit infoResponse(it->objectId, it->info);
      }
    }
    m_info.remove(tid);
    m_transactions.remove(tid);
  }
}

void ChartManager::paintIcon(PickIconData& icon, quint32 chartId, quint32 objectIndex) const {
  if (!m_chartIds.contains(chartId)) return;
  m_charts[m_chartIds[chartId]]->paintIcon(icon, objectIndex);
}

