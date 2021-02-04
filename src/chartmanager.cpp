#include "chartmanager.h"
#include <QFile>
#include <QDir>
#include <QTextStream>
#include <QDebug>
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

ChartManager* ChartManager::instance() {
  static ChartManager* m = new ChartManager();
  return m;
}

ChartManager::ChartManager(QObject *parent)
  : QObject(parent)
  , m_db()
  , m_workers({nullptr}) // temporary, to be replaced in createThreads
  , m_reader(nullptr)
  , m_coverCache(100 * sizeof(ChartCover))
{

  loadPlugins();

  m_readers.append(new CacheReader);

  // create readers
  QSqlQuery r = m_db.exec("select name from chartsets");
  while (r.next()) {
    const QString name = r.value(0).toString();
    if (!m_factories.contains(name)) continue;
    auto reader = m_factories[name]->loadReader();
    if (reader == nullptr) continue;
    m_chartSets[m_factories[name]->displayName()] = m_readers.size();
    m_readers.append(reader);
  }
}

void ChartManager::loadPlugins() {
  const auto& staticFactories = QPluginLoader::staticInstances();
  for (auto plugin: staticFactories) {
    auto factory = qobject_cast<ChartFileReaderFactory*>(plugin);
    if (!factory) continue;
    qDebug() << "Loaded reader plugin" << factory->name();
    m_factories[factory->name()] = factory;
  }

  const QString loc = qAppName().split("_").first();
  const QString base = QLibraryInfo::location(QLibraryInfo::PluginsPath);
  QDir pluginsDir(QString("%1/%2").arg(base).arg(loc));
  qDebug() << "Searching reader plugins in" << pluginsDir.dirName();

  const QStringList plugins = pluginsDir.entryList(QStringList(),
                                                   QDir::Files | QDir::Readable);
  for (auto& plugin: plugins) {
    QPluginLoader loader(pluginsDir.absoluteFilePath(plugin));
    auto factory = qobject_cast<ChartFileReaderFactory*>(loader.instance());
    if (!factory) continue;
    qDebug() << "Loaded reader plugin" << factory->name();
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

void ChartManager::setChartSet(const QString &name, const GeoProjection* vproj) {

  if (!m_chartSets.contains(name)) {
    qWarning() << "Unknown chartset" << name;
    return;
  }
  if (m_reader == m_readers[m_chartSets[name]]) {
    qDebug() << "Current chartset is already" << name;
    return;
  }

  qDebug() << "Changing chartset" << name;

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
  qDebug() << "number of chart updaters =" << numThreads;
  m_workers.clear(); // remove the temporary setting in ctor
  for (int i = 0; i < numThreads; ++i) {
    qDebug() << "creating thread" << i;
    auto thread = new GL::Thread(ctx);
    qDebug() << "creating worker" << i;
    auto worker = new ChartUpdater(m_workers.size());
    m_idleStack.push(worker->id());
    qDebug() << "moving worker to thread" << i;
    worker->moveToThread(thread);
    connect(thread, &QThread::finished, worker, &QObject::deleteLater);
    connect(worker, &ChartUpdater::done, this, &ChartManager::manageThreads);
    qDebug() << "starting thread" << i;
    thread->start();
    m_threads.append(thread);
    m_workers.append(worker);
  }

  m_cacheThread = new GL::Thread(ctx->shareContext());
  m_cacheWorker = new ChartUpdater(m_workers.size());
  m_cacheWorker->moveToThread(m_cacheThread);
  connect(m_cacheThread, &QThread::finished, m_cacheWorker, &QObject::deleteLater);
  m_cacheThread->start();
}

ChartManager::~ChartManager() {
  for (GL::Thread* thread: m_threads) {
    thread->quit();
    thread->wait();
  }
  qDeleteAll(m_threads);

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
                               "where c.chart_id=?");
    r.bindValue(0, chart_id);
    m_db.exec(r);
    int prev = -1;
    int type_id = -1;
    Region cov;
    Region nocov;
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

    if (cov.isEmpty()) {
      WGS84PointVector ws;
      ws << sw;
      ws << WGS84Point::fromLL(ne.lng(), sw.lat());
      ws << ne;
      ws << WGS84Point::fromLL(sw.lng(), ne.lat());

      cov << ws;
    }

    m_coverCache.insert(chart_id, new ChartCover(cov, nocov, p));
  }

  return m_coverCache[chart_id];
}

void ChartManager::updateCharts(const Camera *cam, bool force) {

  if (m_idleStack.size() != m_workers.size()) return;

  m_viewport = m_viewport.translated(cam->geoprojection()->fromWGS84(m_ref));
  m_ref = cam->eye();

  const QRectF vp = cam->boundingBox();
  // qDebug() << "ChartManager::updateCharts" << vp;
  if (!vp.isValid()) return;

  if (m_viewport.contains(vp) && cam->scale() == m_scale && !force) return;

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
  }

  std::sort(scaleCandidates.begin(), scaleCandidates.end(), [this] (quint32 a, quint32 b) {
    const double la = std::abs(log(static_cast<double>(a) / m_scale));
    const double lb = std::abs(log(static_cast<double>(b) / m_scale));
    return la < lb;
  });

  qDebug() << "scale candidates" << scaleCandidates;

  const WGS84Point sw0 = cam->geoprojection()->toWGS84(m_viewArea.topLeft()); // inverted y-axis
  const WGS84Point ne0 = cam->geoprojection()->toWGS84(m_viewArea.bottomRight()); // inverted y-axis

  IDVector chartids;
  for (quint32 selectedScale: scaleCandidates) {
    chartids.clear();

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
    bool covered = false;
    while (r.next()) {
      chartids.append(r.value(0).toUInt());
      qDebug() << chartids.last();
      if (!covered) {
        auto sw = WGS84Point::fromLL(r.value(1).toDouble(), r.value(2).toDouble());
        auto ne = WGS84Point::fromLL(r.value(3).toDouble(), r.value(4).toDouble());
        const ChartCover* c = getCover(chartids.last(), sw, ne, cam->geoprojection());
        covered = c->covers(m_viewArea.center(), cam->geoprojection());
      }
    }
    qDebug() << "chart cover is" << covered;
    qDebug() << "Number of charts" << chartids.size();
    qDebug() << "Nominal scale" << selectedScale;
    qDebug() << "True scale   " << m_scale;
    // select next scale if there's no coverage
    if (covered) {
      break;
    }
  }

  IDVector newCharts;
  for (int id: chartids) {
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
    const WGS84Point sw = cam->geoprojection()->toWGS84(m_viewArea.topLeft());
    const WGS84Point ne = cam->geoprojection()->toWGS84(m_viewArea.bottomRight());
    m_pendingStack.push(ChartData(c, m_scale, sw, ne));
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
      qDebug() << "New chart" << path;
      const WGS84Point sw = cam->geoprojection()->toWGS84(m_viewArea.topLeft());
      const WGS84Point ne = cam->geoprojection()->toWGS84(m_viewArea.bottomRight());
      m_pendingStack.push(ChartData(id, path, m_scale, sw, ne));
    }
  }

  while (!m_pendingStack.isEmpty() && !m_idleStack.isEmpty()) {
    const ChartData d = m_pendingStack.pop();
    const quint32 index = m_idleStack.pop();
    ChartUpdater* dest = m_workers[index];
    if (d.chart != nullptr) {
      QMetaObject::invokeMethod(dest, "updateChart",
                                Q_ARG(S57Chart*, d.chart),
                                Q_ARG(quint32, d.scale),
                                Q_ARG(const WGS84Point&, d.sw),
                                Q_ARG(const WGS84Point&, d.ne));
    } else {
      QMetaObject::invokeMethod(dest, "createChart",
                                Q_ARG(quint32, d.id),
                                Q_ARG(const QString&, d.path),
                                Q_ARG(quint32, d.scale),
                                Q_ARG(const WGS84Point&, d.sw),
                                Q_ARG(const WGS84Point&, d.ne));
    }
  }

  if (noCharts) {
    qDebug() << "ChartManager::updateCharts: idle";
    emit idle();
  }
}



void ChartManager::manageThreads(S57Chart* chart) {
  // qDebug() << "chartmanager: manageThreads";

  if (chart != nullptr) {

    chart->finalizePaintData();

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
                                Q_ARG(S57Chart*, d.chart),
                                Q_ARG(quint32, d.scale),
                                Q_ARG(const WGS84Point&, d.sw),
                                Q_ARG(const WGS84Point&, d.ne));
    } else {
      QMetaObject::invokeMethod(dest, "createChart",
                                Q_ARG(quint32, d.id),
                                Q_ARG(const QString&, d.path),
                                Q_ARG(quint32, d.scale),
                                Q_ARG(const WGS84Point&, d.sw),
                                Q_ARG(const WGS84Point&, d.ne));
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
      qDebug() << "chartmanager: manageThreads: active";
      emit active();
    } else {
      // qDebug() << "chartmanager: manageThreads: charts updated";
      emit chartsUpdated(m_viewArea);
    }
  }
}


void ChartUpdater::createChart(quint32 id, const QString &path, quint32 scale,
                               const WGS84Point& sw, const WGS84Point& ne) {
  try {
    auto chart = new S57Chart(id, path);
    // qDebug() << "ChartUpdater::createChart";
    chart->updatePaintData(sw, ne, scale);
    emit done(chart);
  } catch (ChartFileError& e) {
    qWarning() << "Chart creation failed:" << e.msg();
    emit done(nullptr);
  }
}

void ChartUpdater::updateChart(S57Chart *chart, quint32 scale,
                               const WGS84Point& sw, const WGS84Point& ne) {
  chart->updatePaintData(sw, ne, scale);
  emit done(chart);
}

void ChartUpdater::cacheChart(S57Chart *chart) {
  auto scoped = QScopedPointer<S57Chart>(chart);

  const auto id = CacheReader::CacheId(chart->path());
  const auto base = QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation);

  const auto cachePath = QString("%1/%2/%3").arg(base).arg(qAppName()).arg(QString(id));

  if (!QFileInfo(cachePath).exists()) {
    // not found - cache
    if (!QDir().mkpath(QString("%1/%2").arg(base).arg(qAppName()))) return;
    QFile file(cachePath);
    if (!file.open(QFile::WriteOnly)) return;
    QDataStream stream(&file);
    stream.setVersion(QDataStream::Qt_5_6);
    stream.setByteOrder(QDataStream::LittleEndian);
    // dummy magic - causes simultaneous read to fail
    stream.writeRawData("00000000", 8);
    scoped->encode(stream);
    // write magic
    file.seek(0);
    stream.writeRawData(id.constData(), 8);
    file.close();
  }
}

