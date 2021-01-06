#include "chartmanager.h"
#include <QFile>
#include <QDir>
#include <QTextStream>
#include <QDebug>
#include "s57chart.h"
#include <QStandardPaths>
#include <QtSql/QSqlError>
#include <QVariant>
#include <QDirIterator>
#include "camera.h"
#include <QOpenGLContext>
#include "glthread.h"
#include "glcontext.h"
#include "osencreader.h"
#include "cm93reader.h"
#include "cm93presentation.h"
#include <QDate>
#include <QScopedPointer>

ChartManager::Database::Database() {
  m_DB = QSqlDatabase::addDatabase("QSQLITE");
  QString loc = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
  loc = QString("%1/qopencpn/charts").arg(loc);
  if (!QDir().mkpath(loc)) {
    throw ChartFileError(QString("cannot create charts directory %1").arg(loc));
  }
  const QString dbfile = QString("%1/charts.db").arg(loc);
  m_DB.setDatabaseName(dbfile);
  m_DB.open();
  QSqlQuery query;
  query.exec("create table if not exists chartsets ("
             "id integer primary key autoincrement, "
             "name text not null, "
             "displayName text not null)");
  query.exec("create table if not exists charts ("
             "id integer primary key autoincrement, "
             "chartset integer not null, "
             "scale integer not null, "
             "swx real not null, "
             "swy real not null, "
             "nex real not null, "
             "ney real not null, "
             "published int not null, " // Julian day
             "modified int not null, "  // Julian day
             "path text not null unique)");
  m_DB.close();
}

const QSqlQuery& ChartManager::Database::exec(const QString& sql) {
  if (!m_DB.isOpen()) m_DB.open();
  m_Query = QSqlQuery(m_DB);
  // qDebug() << sql;
  m_Query.exec(sql);
  if (m_Query.lastError().isValid()) qWarning() << m_Query.lastError();
  return m_Query;
}

void ChartManager::Database::exec(QSqlQuery& query) {
  if (!m_DB.isOpen()) m_DB.open();
  query.exec();
  if (query.lastError().isValid()) qWarning() << query.lastError();
}

const QSqlQuery& ChartManager::Database::prepare(const QString& sql) {
  if (!m_DB.isOpen()) m_DB.open();
  m_Query = QSqlQuery(m_DB);
  // qDebug() << sql;
  m_Query.prepare(sql);
  if (m_Query.lastError().isValid()) qWarning() << m_Query.lastError();
  return m_Query;
}

bool ChartManager::Database::transaction() {
  if (!m_DB.isOpen()) m_DB.open();
  return m_DB.transaction();
}

bool ChartManager::Database::commit() {
  return m_DB.commit();
}

void ChartManager::Database::close() {
  m_DB.commit();
  m_DB.close();
}


ChartManager* ChartManager::instance() {
  static ChartManager* m = new ChartManager();
  return m;
}

ChartManager::ChartManager(QObject *parent)
  : QObject(parent)
  , m_db()
  , m_workers({nullptr}) // temporary, to be replaced in createThreads
  , m_reader(nullptr)
  , m_filters {{"osenc", {"*.S57"}},
               {"cm93", {"*.A", "*.B", "*.C", "*.D", "*.E", "*.F", "*.G", "*.Z"}}}
{
  // check & fill chartsets table
  const QString sql = "select name, displayName from chartsets";
  QSqlQuery r = m_db.exec(sql);
  if (!r.first()) {
    // empty table, fill it
    fillChartsetsTable();
    r = m_db.exec(sql);
  }
  r.seek(-1);
  while (r.next()) {
    const QString name = r.value(0).toString();
    const QString displayName = r.value(1).toString();
    m_chartSets[displayName] = m_readers.size();
    m_readers.append(createReader(name));
  }
}

void ChartManager::fillChartsetsTable() {
  QStringList locs;
  for (const QString& loc: QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation)) {
    locs << QString("%1/qopencpn/charts").arg(loc);
  }
  const QMap<QString, QString> supported {{"cm93", "CM93 Charts"},
                                          {"osenc", "OSENC Charts"}};

  QMap<QString, QString> present;

  for (const QString& loc: locs) {
    QDir dataDir(loc);
    const QStringList dirs = dataDir.entryList(QStringList(),
                                               QDir::Dirs | QDir::Readable);
    for (auto dir: dirs) {
      if (supported.contains(dir)) {
        present[dir] = supported[dir];
      }
    }
  }

  for (auto it = present.cbegin(); it != present.cend(); ++it) {
    // insert
    QSqlQuery r = m_db.prepare("insert into chartsets "
                               "(name, displayName) "
                               "values(?, ?)");
    r.bindValue(0, QVariant::fromValue(it.key()));
    r.bindValue(1, QVariant::fromValue(it.value()));
    m_db.exec(r);
  }
}

ChartFileReader* ChartManager::createReader(const QString &name) const {
  if (name == "osenc") {
    return new OsencReader();
  }
  if (name == "cm93") {
    CM93::InitPresentation();
    return new CM93Reader();
  }
  return nullptr;
}

void ChartManager::fillChartsTable() {

  QStringList locs;
  for (const QString& loc: QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation)) {
    locs << QString("%1/qopencpn/charts/%2").arg(loc).arg(m_reader->name());
  }

  QSqlQuery r1 = m_db.prepare("select id "
                              "from chartsets "
                              "where name=?");
  r1.bindValue(0, QVariant::fromValue(m_reader->name()));
  m_db.exec(r1);
  uint setid = 0;
  while (r1.next()) {
    setid = r1.value(0).toUInt();
  }


  QVector<quint32> oldCharts;
  QSqlQuery r2 = m_db.prepare("select id "
                              "from charts "
                              "where chartset=?");
  r2.bindValue(0, QVariant::fromValue(setid));
  m_db.exec(r2);
  while (r2.next()) {
    oldCharts.append(r2.value(0).toUInt());
  }

  if (!m_db.transaction()) {
    qWarning() << "Cannot create db transaction, not updating";
    return;
  }

  for (const QString& loc: locs) {
    QDirIterator it(loc, m_filters[m_reader->name()], QDir::Files | QDir::Readable,
                    QDirIterator::FollowSymlinks | QDirIterator::Subdirectories);
    while (it.hasNext()) {
      try {
        const QString path = it.next();
        S57ChartOutline ch = m_reader->readOutline(path);
        // skip large scale maps
        if (ch.extent().nw().lat() - ch.extent().sw().lat() > 9.) {
          qDebug() << "Skipping" << ch.extent().sw().print() << ch.extent().ne().print();
          continue;
        }

        QSqlQuery s = m_db.prepare("select id, published, modified from charts where path=?");
        s.bindValue(0, QVariant::fromValue(path));
        m_db.exec(s);
        if (!s.first()) {
          // insert
          QSqlQuery t = m_db.prepare("insert into charts"
                                     "(chartset, scale, swx, swy, nex, ney, "
                                     " published, modified, path) "
                                     "values(?, ?, ?, ?, ?, ?, ?, ?, ?)");
          // chartset, scale (0, 1)
          t.bindValue(0, QVariant::fromValue(setid));
          t.bindValue(1, QVariant::fromValue(ch.scale()));
          // sw, ne (2, 3, 4, 5)
          t.bindValue(2, QVariant::fromValue(ch.extent().sw().lng()));
          t.bindValue(3, QVariant::fromValue(ch.extent().sw().lat()));
          t.bindValue(4, QVariant::fromValue(ch.extent().ne().lng()));
          t.bindValue(5, QVariant::fromValue(ch.extent().ne().lat()));
          // published, modified, path (6, 7, 8)
          t.bindValue(6, QVariant::fromValue(ch.published().toJulianDay()));
          t.bindValue(7, QVariant::fromValue(ch.modified().toJulianDay()));
          t.bindValue(8, QVariant::fromValue(path));

          m_db.exec(t);
        } else {
          oldCharts.removeOne(s.value(0).toInt());
          if (ch.published().toJulianDay() == s.value(1).toInt() &&
              ch.modified().toJulianDay() == s.value(2).toInt()) {
            continue;
          }
          // update
          QSqlQuery t = m_db.prepare("update charts set "
                                     "scale=?, swx=?, swy=?, nex=?, ney=?, "
                                     "published=?, modified=? "
                                     "where id=?");
          // scale (0)
          t.bindValue(0, QVariant::fromValue(ch.scale()));
          // sw, ne (1, 2, 3, 4)
          t.bindValue(1, QVariant::fromValue(ch.extent().sw().lng()));
          t.bindValue(2, QVariant::fromValue(ch.extent().sw().lat()));
          t.bindValue(3, QVariant::fromValue(ch.extent().ne().lng()));
          t.bindValue(4, QVariant::fromValue(ch.extent().ne().lat()));
          // published, modified, path (5, 6)
          t.bindValue(5, QVariant::fromValue(ch.published().toJulianDay()));
          t.bindValue(6, QVariant::fromValue(ch.modified().toJulianDay()));
          // id (7)
          t.bindValue(7, s.value(0));

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
  m_locationAreas.clear();
  m_outlines.clear();

  m_ref = vproj->reference();

  // check & fill charts table
  QSqlQuery r = m_db.prepare("select c.id, c.scale, c.swx, c.swy, c.nex, c.ney "
                             "from charts c "
                             "join chartsets s "
                             "on c.chartset=s.id "
                             "where s.name=?");
  r.bindValue(0, QVariant::fromValue(m_reader->name()));
  m_db.exec(r);
  if (!r.first()) {
    // empty table, fill it
    fillChartsTable();
    m_db.exec(r);
  }

  QScopedPointer<GeoProjection> p = QScopedPointer<GeoProjection>(GeoProjection::CreateProjection(vproj->className()));
  r.seek(-1);
  // read all chart rows & create location areas
  while (r.next()) {
    ChartInfo info;
    info.id = r.value(0).toInt();
    info.scale = r.value(1).toInt();
    auto sw = WGS84Point::fromLL(r.value(2).toDouble(), r.value(3).toDouble());
    auto ne = WGS84Point::fromLL(r.value(4).toDouble(), r.value(5).toDouble());
    info.ref = sw + .5 * (ne - sw);
    p->setReference(info.ref);
    info.bbox = QRectF(p->fromWGS84(sw), p->fromWGS84(ne));
    createOutline(sw, ne);

    auto it = m_locationAreas.begin();
    for (; it != m_locationAreas.end(); ++it) {
      const float d = (info.ref - it->charts.first().ref).meters();
      if (d < locationRadius) {
        it->charts.append(info);
        break;
      }
    }
    if (it == m_locationAreas.end()) {
      LocationArea area;
      area.charts.append(info);
      m_locationAreas.append(area);
    }
  }

  // create location area bounding boxes.
  for (auto it = m_locationAreas.begin(); it != m_locationAreas.end(); ++it) {
    it->bbox = it->charts.first().bbox;
    p->setReference(it->charts.first().ref);
    for (const ChartInfo& chart: it->charts) {
      it->bbox |= chart.bbox.translated(p->fromWGS84(chart.ref));
    }
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
  const int numThreads = QThread::idealThreadCount() - 1;
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
  if (!vp.isValid()) return;
  // qDebug() << "ChartManager::updateCharts" << vp;

  if (m_viewport.contains(vp) && cam->scale() == m_scale && !force) return;

  m_scale = cam->scale();

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

  double distmin = 1.e15;
  quint32 bestScale = 0;
  QMap<quint32, IDVector> scaleBin;
  for (const LocationArea& a: m_locationAreas) {
    const WGS84Point ref = a.charts.first().ref;
    const float d = (m_ref - ref).meters();
    if (d > maxRadius) continue; // this test removes dateline and similar problems
    if (!m_viewArea.intersects(a.bbox.translated(cam->geoprojection()->fromWGS84(ref)))) continue;
    for (const ChartInfo& chart: a.charts) {
      if (!m_viewArea.intersects(chart.bbox.translated(cam->geoprojection()->fromWGS84(chart.ref))))  continue;
      if (chart.scale > maxScaleRatio * m_scale) continue;
      if (m_scale > maxScaleRatio * chart.scale) continue;

      const double dist = std::abs(log(static_cast<double>(m_scale) / chart.scale));
      if (dist < distmin) {
        distmin = dist;
        bestScale = chart.scale;
      }
      scaleBin[chart.scale].append(chart.id);

    }
  }

  IDVector newCharts;
  for (int id: scaleBin[bestScale]) {
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
      QMetaObject::invokeMethod(dest, [dest, d] () {
        dest->updateChart(d.chart, d.scale, d.sw, d.ne);
      });
    } else {
      QMetaObject::invokeMethod(dest, [dest, d] () {
        dest->createChart(d.id, d.path, d.scale, d.sw, d.ne);
      });
    }
  }

  if (noCharts && m_hadCharts) {
    qDebug() << "ChartManager::updateCharts: idle";
    emit idle();
  }
}

bool ChartManager::isValidScale(const Camera* cam, quint32 scale) const {

  if (m_charts.isEmpty()) return true;

  QScopedPointer<GeoProjection> p = QScopedPointer<GeoProjection>(
        GeoProjection::CreateProjection(cam->geoprojection()->className()));

  p->setReference(m_ref);
  const QRectF vp = cam->boundingBox().translated(p->fromWGS84(cam->eye()));

  for (const LocationArea& a: m_locationAreas) {
    const WGS84Point ref = a.charts.first().ref;
    const float d = (m_ref - ref).meters();
    if (d > maxRadius) continue; // this test removes dateline and similar problems
    if (!vp.intersects(a.bbox.translated(p->fromWGS84(ref)))) continue;
    for (const ChartInfo& chart: a.charts) {
      if (!vp.intersects(chart.bbox.translated(p->fromWGS84(chart.ref))))  continue;
      if (float(chart.scale) / scale > maxScaleRatio) continue;
      if (float(scale) / chart.scale > maxScaleRatio) continue;
      return true;
    }
  }

  return false;
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
      QMetaObject::invokeMethod(dest, [dest, d] () {
        dest->updateChart(d.chart, d.scale, d.sw, d.ne);
      });
    } else {
      QMetaObject::invokeMethod(dest, [dest, d] () {
        dest->createChart(d.id, d.path, d.scale, d.sw, d.ne);
      });
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

