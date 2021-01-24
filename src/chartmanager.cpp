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
#include "glcontext.h"
#include "chartfilereader.h"
#include <QDate>
#include <QScopedPointer>
#include <QRegion>
#include <QPluginLoader>
#include <QLibraryInfo>


ChartManager* ChartManager::instance() {
  static ChartManager* m = new ChartManager();
  return m;
}

ChartManager::ChartManager(QObject *parent)
  : QObject(parent)
  , m_db()
  , m_workers({nullptr}) // temporary, to be replaced in createThreads
  , m_reader(nullptr)
{

  loadPlugins();

  // check & fill chartsets table
  const QString sql = "select name from chartsets";
  QSqlQuery r = m_db.exec(sql);
  if (!r.first()) {
    // empty table, fill it
    fillChartsetsTable();
    r = m_db.exec(sql);
  }
  r.seek(-1);
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

  QString base = QLibraryInfo::location(QLibraryInfo::PluginsPath);
  QDir pluginsDir(QString("%1/%2").arg(base).arg(qAppName()));
  qDebug() << "Searching reader plugins in" << pluginsDir;

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

void ChartManager::fillChartsetsTable() {
  QStringList locs;
  for (const QString& loc: QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation)) {
    locs << QString("%1/%2/charts").arg(loc).arg(qAppName());
  }

  QStringList present;

  for (const QString& loc: locs) {
    QDir dataDir(loc);
    const QStringList dirs = dataDir.entryList(QStringList(),
                                               QDir::Dirs | QDir::Readable);
    for (auto dir: dirs) {
      if (m_factories.contains(dir)) {
        present << dir;
      }
    }
  }

  for (auto name: present) {
    // insert
    QSqlQuery r = m_db.prepare("insert into main.chartsets (name) values(?)");
    r.bindValue(0, name);
    m_db.exec(r);
  }
}


void ChartManager::fillScalesAndChartsTables() {

  QStringList locs;
  for (const QString& loc: QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation)) {
    locs << QString("%1/%2/charts/%3").arg(loc).arg(qAppName()).arg(m_reader->name());
  }

  QSqlQuery r1 = m_db.prepare("select id "
                              "from main.chartsets "
                              "where name=?");
  r1.bindValue(0, QVariant::fromValue(m_reader->name()));
  m_db.exec(r1);
  r1.first();
  uint set_id = r1.value(0).toUInt();


  QVector<quint32> oldCharts;
  QSqlQuery r2 = m_db.prepare("select c.id "
                              "from main.charts c "
                              "join main.scales s on c.scale_id = s.id "
                              "where s.chartset_id=?");
  r2.bindValue(0, QVariant::fromValue(set_id));
  m_db.exec(r2);
  while (r2.next()) {
    oldCharts.append(r2.value(0).toUInt());
  }


  ScaleMap scales;
  QSqlQuery r3 = m_db.prepare("select s.id, s.scale "
                              "from main.scales s "
                              "where chartset_id=?");
  r3.bindValue(0, set_id);
  m_db.exec(r3);
  while (r3.next()) {
    scales[r3.value(1).toUInt()] = r3.value(0).toUInt();
  }


  if (!m_db.transaction()) {
    qWarning() << "Cannot create db transaction, not updating";
    return;
  }

  for (const QString& loc: locs) {
    QDirIterator it(loc,
                    m_factories[m_reader->name()]->filters(),
                    QDir::Files | QDir::Readable,
                    QDirIterator::FollowSymlinks | QDirIterator::Subdirectories);
    while (it.hasNext()) {
      try {
        const QString path = it.next();
        S57ChartOutline ch = m_reader->readOutline(path);
        if (!scales.contains(ch.scale())) {
          auto r4 = m_db.prepare("insert into main.scales"
                                 "(chartset_id, scale) "
                                 "values(?, ?)");
          r4.bindValue(0, set_id);
          r4.bindValue(1, ch.scale());
          m_db.exec(r4);
          scales[ch.scale()] = r4.lastInsertId().toUInt();
        }

        // skip large scale maps
        //        if (ch.extent().nw().lat() - ch.extent().sw().lat() > 9.) {
        //          qDebug() << "Skipping" << ch.extent().sw().print() << ch.extent().ne().print();
        //          continue;
        //        }

        QSqlQuery s = m_db.prepare("select id, published, modified "
                                   "from main.charts where path=?");
        s.bindValue(0, path);
        m_db.exec(s);
        if (!s.first()) {
          // insert
          QSqlQuery t = m_db.prepare("insert into main.charts"
                                     "(scale_id, swx, swy, nex, ney, "
                                     " published, modified, path) "
                                     "values(?, ?, ?, ?, ?, ?, ?, ?)");
          // scale_id (0)
          t.bindValue(0, QVariant::fromValue(scales[ch.scale()]));
          // sw, ne (1, 2, 3, 4)
          auto sw = ch.extent().sw();
          auto ne = ch.extent().ne();
          t.bindValue(1, sw.lng());
          t.bindValue(2, sw.lat());
          t.bindValue(3, ne.lng(sw));
          t.bindValue(4, ne.lat());
          // published, modified, path (5, 6, 7)
          t.bindValue(5, QVariant::fromValue(ch.published().toJulianDay()));
          t.bindValue(6, QVariant::fromValue(ch.modified().toJulianDay()));
          t.bindValue(7, QVariant::fromValue(path));

          m_db.exec(t);
        } else {
          oldCharts.removeOne(s.value(0).toInt());
          if (ch.published().toJulianDay() == s.value(1).toInt() &&
              ch.modified().toJulianDay() == s.value(2).toInt()) {
            continue;
          }
          // update
          QSqlQuery t = m_db.prepare("update main.charts set "
                                     "swx=?, swy=?, nex=?, ney=?, "
                                     "published=?, modified=? "
                                     "where id=?");
          // sw, ne (0, 1, 2, 3)
          t.bindValue(0, QVariant::fromValue(ch.extent().sw().lng()));
          t.bindValue(1, QVariant::fromValue(ch.extent().sw().lat()));
          t.bindValue(2, QVariant::fromValue(ch.extent().ne().lng()));
          t.bindValue(3, QVariant::fromValue(ch.extent().ne().lat()));
          // published, modified, path (4, 5)
          t.bindValue(4, QVariant::fromValue(ch.published().toJulianDay()));
          t.bindValue(5, QVariant::fromValue(ch.modified().toJulianDay()));
          // id (6)
          t.bindValue(6, s.value(0));

          m_db.exec(t);
        }
      } catch (ChartFileError& e) {
        qWarning() << "Chart file error:" << e.msg() << ", skipping";
      }
    }
  }

  // remove old rows
  if (!oldCharts.isEmpty()) {
    QString sql = "delete from charts where id in (";
    sql += QString("?,").repeated(oldCharts.size());
    sql = sql.replace(sql.length() - 1, 1, ")");
    QSqlQuery s = m_db.prepare(sql);
    for (int i = 0; i < oldCharts.size(); i++) {
      s.bindValue(i, QVariant::fromValue(oldCharts[i]));
    }
    m_db.exec(s);
  }

  if (!m_db.commit()) {
    qWarning() << "DB commit failed!";
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
  if (!r.first()) {
    // empty table, fill it
    fillScalesAndChartsTables();
    m_db.exec(r);
  }

  r.seek(-1);
  while (r.next()) {
    m_scales.append(r.value(0).toUInt());
  }
  std::sort(m_scales.begin(), m_scales.end());

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
    auto thread = new GL::Thread(ctx);
    auto worker = new ChartUpdater(m_workers.size());
    m_idleStack.push(worker->id());
    worker->moveToThread(thread);
    connect(thread, &QThread::finished, worker, &QObject::deleteLater);
    connect(worker, &ChartUpdater::done, this, &ChartManager::manageThreads);
    thread->start();
    m_threads.append(thread);
    m_workers.append(worker);
  }
}

ChartManager::~ChartManager() {
  for (GL::Thread* thread: m_threads) {
    thread->quit();
    thread->wait();
  }
  qDeleteAll(m_threads);
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

  IDVector chartids;
  for (quint32 selectedScale: scaleCandidates) {
    chartids.clear();

    // select charts
    auto sw0 = cam->geoprojection()->toWGS84(m_viewArea.topLeft()); // inverted y-axis
    auto ne0 = cam->geoprojection()->toWGS84(m_viewArea.bottomRight()); // inverted y-axis
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
    QRegion cover;
    while (r.next()) {
      chartids.append(r.value(0).toUInt());
      auto sw = WGS84Point::fromLL(r.value(1).toDouble(), r.value(2).toDouble());
      auto ne = WGS84Point::fromLL(r.value(3).toDouble(), r.value(4).toDouble());
      auto p1 = cam->geoprojection()->fromWGS84(sw);
      auto p2 = cam->geoprojection()->fromWGS84(ne);
      cover += QRect(p1.toPoint(), p2.toPoint());
    }
    qDebug() << "chart cover is" << cover.contains(m_viewArea.toRect().center());
    qDebug() << "Number of charts" << chartids.size();
    qDebug() << "Nominal scale" << S52::PrintScale(selectedScale);
    qDebug() << "True scale" << S52::PrintScale(m_scale);
    // select next scale if there's no coverage
    if (cover.contains(m_viewArea.toRect().center())) {
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

  // remove old charts
  for (auto it = m_chartIds.constBegin(); it != m_chartIds.constEnd(); ++it) {
    delete m_charts[it.value()];
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

    GL::Context::instance()->makeCurrent();
    chart->finalizePaintData();
    GL::Context::instance()->doneCurrent();

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

