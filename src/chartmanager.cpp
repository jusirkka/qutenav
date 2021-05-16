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
#include "logging.h"
#include "s57chart.h"
#include <QStandardPaths>
#include <QVariant>
#include <QDirIterator>
#include "camera.h"
#include <QOpenGLContext>
#include "glthread.h"
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

ChartManager* ChartManager::instance() {
  static ChartManager* m = new ChartManager();
  return m;
}

ChartManager::ChartManager(QObject *parent)
  : QObject(parent)
  , m_db()
  , m_workers({nullptr}) // temporary, to be replaced in createThreads
  , m_transactionCounter(0)
  , m_reader(nullptr)
  , m_updater(new UpdaterInterface(this))
  , m_coverCache(100 * sizeof(ChartCover))
{

  loadPlugins();

  m_readers.append(new CacheReader);

  // create readers & chartsets
  updateChartSets();

  connect(m_updater, &UpdaterInterface::ready, this, &ChartManager::updateChartSets);
}

void ChartManager::updateChartSets() {

  qCDebug(CMGR) << "updateChartSets";

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
  QSqlQuery r = m_db.prepare("select scale "
                             "from main.scales "
                             "where chartset_id=?");
  r.bindValue(0, set_id);
  m_db.exec(r);
  while (r.next()) {
    m_scales.append(r.value(0).toUInt());
  }
  std::sort(m_scales.begin(), m_scales.end());

  // load charts to attached memory db
  m_db.loadCharts(set_id);

  // outlines
  QSqlQuery s = m_db.exec("select swx, swy, nex, ney "
                          "from m.charts");
  while (s.next()) {
    auto sw = WGS84Point::fromLL(s.value(0).toDouble(), s.value(1).toDouble());
    auto ne = WGS84Point::fromLL(s.value(2).toDouble(), s.value(3).toDouble());
    createOutline(sw, ne);
  }
}

void ChartManager::createOutline(const WGS84Point& sw, const WGS84Point& ne) {
  m_outlines.append(sw.lng());
  m_outlines.append(sw.lat());
  m_outlines.append(ne.lng());
  m_outlines.append(sw.lat());
  m_outlines.append(ne.lng());
  m_outlines.append(ne.lat());
  m_outlines.append(sw.lng());
  m_outlines.append(ne.lat());
}

void ChartManager::createThreads(QOpenGLContext* ctx) {
  const int numThreads = qMax(1, QThread::idealThreadCount() - 1);
  qCDebug(CMGR) << "number of chart updaters =" << numThreads;
  m_workers.clear(); // remove the temporary setting in ctor
  for (int i = 0; i < numThreads; ++i) {
    qCDebug(CMGR) << "creating thread" << i;
    auto thread = new GL::Thread(ctx);
    qCDebug(CMGR) << "creating worker" << i;
    auto worker = new ChartUpdater(m_workers.size());
    m_idleStack.push(worker->id());
    qCDebug(CMGR) << "moving worker to thread" << i;
    worker->moveToThread(thread);
    connect(thread, &QThread::finished, worker, &QObject::deleteLater);
    connect(worker, &ChartUpdater::done, this, &ChartManager::manageThreads);
    connect(worker, &ChartUpdater::infoResponse, this, &ChartManager::manageInfoResponse);
    qCDebug(CMGR) << "starting thread" << i;
    thread->start();
    m_threads.append(thread);
    m_workers.append(worker);
  }

  m_cacheThread = new GL::Thread(ctx);
  m_cacheWorker = new ChartUpdater(m_workers.size());
  m_cacheWorker->moveToThread(m_cacheThread);
  connect(m_cacheThread, &QThread::finished, m_cacheWorker, &QObject::deleteLater);
  m_cacheThread->start();
  qCDebug(CMGR) << "threads started";
}

ChartManager::~ChartManager() {
  for (GL::Thread* thread: m_threads) {
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
                                         const GeoProjection *p) {
  if (!m_coverCache.contains(chart_id)) {
    QSqlQuery r = m_db.prepare("select c.id, c.type_id, p.x, p.y "
                               "from m.polygons p "
                               "join m.coverage c on p.cov_id = c.id "
                               "where c.chart_id=? order by c.id, p.id");
    r.bindValue(0, chart_id);
    m_db.exec(r);
    int prev = -1;
    int type_id = -1;
    LLPolygon cov;
    LLPolygon nocov;
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

    auto c = new ChartCover(cov, nocov, sw, ne, p);
    // chartcover::tognuplot(cov, nocov, c->region(p), sw, ne, p, chart_id);
    m_coverCache.insert(chart_id, c);
  }

  return m_coverCache[chart_id];
}

void ChartManager::updateCharts(const Camera *cam, quint32 flags) {

  if (m_idleStack.size() != m_workers.size()) return;

  m_viewport = m_viewport.translated(cam->geoprojection()->fromWGS84(m_ref));
  m_ref = cam->eye();

  const QRectF vp = cam->boundingBox();
  // qCDebug(CMGR) << "ChartManager::updateCharts" << vp;
  if (!vp.isValid()) return;

  if (m_viewport.contains(vp) && cam->scale() == m_scale && flags == 0) return;

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
    // move small scales to end of candidate list in reversed order
    ScaleVector smallScales;
    while (index > 0) {
      smallScales.prepend(scaleCandidates.takeFirst());
      index--;
    }
    scaleCandidates.append(smallScales);
  }

  qCDebug(CMGR) << "target" << m_scale << ", candidates" << scaleCandidates;

  const WGS84Point sw0 = cam->geoprojection()->toWGS84(m_viewArea.topLeft()); // inverted y-axis
  const WGS84Point ne0 = cam->geoprojection()->toWGS84(m_viewArea.bottomRight()); // inverted y-axis


  KV::Region remainingArea(m_viewArea);
  KV::RegionMap regions;
  KV::RegionMap covers;

  const auto totarea = remainingArea.area();
  qreal noncov = 100;

  for (quint32 selectedScale: scaleCandidates) {

    // select charts
    QSqlQuery r = m_db.prepare("select chart_id, swx, swy, nex, ney, path "
                               "from m.charts "
                               "where scale = ? and "
                               "      swx < ? and swy < ? and "
                               "      nex > ? and ney > ?");
    r.bindValue(0, selectedScale);
    r.bindValue(1, ne0.lng(sw0));
    r.bindValue(2, ne0.lat());
    r.bindValue(3, sw0.lng());
    r.bindValue(4, sw0.lat());

    m_db.exec(r);
    while (r.next() && noncov >= .1) {
      quint32 id = r.value(0).toUInt();
      auto sw = WGS84Point::fromLL(r.value(1).toDouble(), r.value(2).toDouble());
      auto ne = WGS84Point::fromLL(r.value(3).toDouble(), r.value(4).toDouble());
      auto c = getCover(id, sw, ne, cam->geoprojection());
      auto reg = c->region(cam->geoprojection()) & remainingArea;
      if (reg.isValid()) {
        remainingArea -= reg;
        noncov = remainingArea.area() / totarea * 100;
        regions[id] = reg;
        covers[id] = c->region(cam->geoprojection());
        qCDebug(CMGR) << "chart" << id << selectedScale << ", covers" << reg.area() / totarea * 100
                 << ", remaining" << noncov;
      }
    }
    if (noncov < .1) {
      break;
    }
  }

  // chartmanager::tognuplot(regions, m_viewArea, "regions");
  // chartmanager::tognuplot(covers, m_viewArea, "covers");

  qCDebug(CMGR) << "Number of charts" << regions.size()
           << ", covered =" << (noncov < 1.);



  IDVector newCharts;
  for (auto id: regions.keys()) {
    if (!m_chartIds.contains(id)) {
      newCharts.append(id);
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

  bool noCharts = m_charts.isEmpty() && newCharts.isEmpty();
  // create pending chart update data
  for (S57Chart* c: m_charts) {
    // Note: inverted y-axis
    m_pendingStack.push(ChartData(c, m_scale, regions[c->id()].toWGS84(cam->geoprojection()),
                        (flags & UpdateLookups) != 0));
  }
  // create pending chart creation data
  if (!newCharts.isEmpty()) {
    QString sql = "select id, path from charts where id in (";
    sql += QString("?,").repeated(newCharts.size());
    sql = sql.replace(sql.length() - 1, 1, ")");
    QSqlQuery r = m_db.prepare(sql);
    for (int i = 0; i < newCharts.size(); i++) {
      r.bindValue(i, QVariant::fromValue(newCharts[i]));
    }
    m_db.exec(r);
    while (r.next()) {
      const quint32 id = r.value(0).toUInt();
      const auto path = r.value(1).toString();
      qCDebug(CMGR) << "New chart" << path;
      m_pendingStack.push(ChartData(id, path, m_scale,
                                    regions[id].toWGS84(cam->geoprojection())));
    }
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



void ChartManager::manageThreads(S57Chart* chart) {
  // qCDebug(CMGR) << "chartmanager: manageThreads";

  if (chart != nullptr) {
    auto chartId = chart->id();
    if (!m_chartIds.contains(chartId)) {
      m_chartIds[chartId] = m_charts.size();
      m_charts.append(chart);
    }
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


  if (m_idleStack.size() == m_workers.size()) {
    if (!m_hadCharts) {
      qCDebug(CMGR) << "chartmanager: manageThreads: active";
      emit active();
    } else {
      // qCDebug(CMGR) << "chartmanager: manageThreads: charts updated";
      emit chartsUpdated(m_viewArea);
    }
  }
}

void ChartManager::requestInfo(const WGS84Point &p) {

  QString sql("select chart_id, swx, swy, nex, ney "
              "from m.charts "
              "where chart_id in (");
  sql += QString("?,").repeated(m_charts.size());
  sql = sql.replace(sql.length() - 1, 1, ")");

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
    qInfo() << p.print() << "is outside of area covered by current chartset";
    return;
  }

  m_transactions[m_transactionCounter] = m_charts.size() - reqs.size();
  int counter = 0;
  for (auto chart: reqs) {
    QMetaObject::invokeMethod(m_workers[counter], "requestInfo",
                              Q_ARG(S57Chart*, chart),
                              Q_ARG(const WGS84Point&, p),
                              Q_ARG(quint32, m_scale),
                              Q_ARG(quint32, m_transactionCounter));
    counter = (counter + 1) % m_workers.size();
  }
  m_transactionCounter += 1;
}

void ChartManager::manageInfoResponse(const S57::InfoType& info, quint32 tid) {
  m_transactions[tid] = m_transactions[tid] + 1;

  if (m_info[tid].isEmpty()) {
    m_info[tid] = info;
  } else {
    qCWarning(CMGR) << "Object info in more than one chart";
  }

  if (m_transactions[tid] == m_charts.size()) {
    S57::InfoType info = m_info[tid];
    m_info.remove(tid);
    m_transactions.remove(tid);
    qCDebug(CMGR) << "ChartManager::manageInfoResponse";
    emit infoResponse(info);
  }
}


