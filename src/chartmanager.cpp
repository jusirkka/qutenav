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
  query.exec("create table if not exists charts ("
             "id integer primary key autoincrement, "
             "scale integer not null, "
             "geoproj text not null, "
             "reference text not null, "
             "width real not null, "
             "height real not null, "
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
  , m_db() {

  // check & fill database
  const QString sql = "select id, scale, geoproj, reference, width, height, path from charts";
  QSqlQuery r = m_db.exec(sql);
  if (!r.first()) {
    // empty table, fill it
    updateDB();
    r = m_db.exec(sql);
  }

  // FIXME: read current geoprojection from config
  m_proj = new SimpleMercator;
  m_proj->setReference(WGS84Point::fromLL(0, 0));

  r.seek(-1);
  SimpleMercator p;
  // read all chart rows & create location areas
  while (r.next()) {
    if (r.value(2).toString() != "SimpleMercator") {
      throw NotImplementedError("Only SimpleMercator supported for now");
    }
    ChartInfo info;
    info.id = r.value(0).toInt();
    info.scale = r.value(1).toInt();
    const float w = r.value(4).toFloat();
    const float h = r.value(5).toFloat();
    info.ref = WGS84Point::parseISO6709(r.value(3).toString());
    info.bbox = QRectF(- QPointF(w / 2, h / 2), QSizeF(w, h));
    createOutline(&p, info);
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
    SimpleMercator p;
    p.setReference(it->charts.first().ref);
    for (const ChartInfo& chart: it->charts) {
      it->bbox |= chart.bbox.translated(p.fromWGS84(chart.ref));
    }
  }
}

void ChartManager::createOutline(GeoProjection* proj, const ChartInfo& info) {
  proj->setReference(info.ref);
  // Note: inverted y-axis
  QVector<QPointF> ps{
    info.bbox.topLeft(),
    info.bbox.topRight(),
    info.bbox.bottomRight(),
    info.bbox.bottomLeft()
  };

  for (const QPointF& p: ps) {
    const WGS84Point q = proj->toWGS84(p);
    m_outlines.append(q.lng());
    m_outlines.append(q.lat());
  }
}

void ChartManager::updateCharts(const Camera *cam) {

  if (!m_threads.isEmpty()) return;

  m_viewport = m_viewport.translated(cam->geoprojection()->fromWGS84(m_proj->reference()));
  m_proj->setReference(cam->geoprojection()->reference());

  const QRectF vp = cam->boundingBox();

  const auto margin = QMarginsF(vp.width(), vp.height(), vp.width(), vp.height());
  bool vpok = m_viewport.contains(vp) && !m_viewport.contains(vp + margin);
  if (vpok && cam->scale() == m_scale) return;

  m_scale = cam->scale();

  if (!vpok) {
    const float mw = vp.width() * (viewportFactor - 1) / 2;
    const float mh = vp.height() * (viewportFactor - 1) / 2;
    m_viewport = vp + QMarginsF(mw, mh, mw, mh);
  }
  const float mw = m_viewport.width() * (marginFactor - 1) / 2;
  const float mh = m_viewport.height() * (marginFactor - 1) / 2;
  auto viewArea = m_viewport + QMarginsF(mw, mh, mw, mh);

  IDVector newCharts;
  for (const LocationArea& a: m_locationAreas) {
    const WGS84Point ref = a.charts.first().ref;
    const float d = (m_proj->reference() - ref).meters();
    if (d > maxRadius) continue; // this test removes dateline and similar problems
    if (!viewArea.intersects(a.bbox.translated(m_proj->fromWGS84(ref)))) continue;
    for (const ChartInfo& chart: a.charts) {
      if (!viewArea.intersects(chart.bbox.translated(m_proj->fromWGS84(chart.ref))))  continue;
      if (float(chart.scale) / m_scale > maxScaleRatio) continue;
      if (float(m_scale) / chart.scale > maxScaleRatio) continue;
      if (!m_ids.contains(chart.id)) {
        newCharts.append(chart.id);
      } else {
        m_ids.remove(chart.id);
      }
    }
  }

  m_hadCharts = !m_charts.isEmpty();
  m_onlyObjects = newCharts.isEmpty() && m_ids.isEmpty();

  // remove old charts
  for (auto it = m_ids.constBegin(); it != m_ids.constEnd(); ++it) {
    delete m_charts[it.value()];
    m_charts[it.value()] = nullptr;
  }
  ChartVector charts;
  // update m_ids
  m_ids.clear();
  for (S57Chart* c: m_charts) {
    if (c != nullptr) {
      m_ids[c->id()] = charts.size();
      charts.append(c);
    }
  }
  m_charts = charts;

  bool noCharts = m_charts.isEmpty() && newCharts.isEmpty();
  // create updater(object) threads
  for (S57Chart* c: m_charts) {
    const QRectF va = viewArea.translated(c->geoProjection()->fromWGS84(m_proj->reference()));
    auto upd = new ChartUpdater(c, va, m_scale);
    connect(upd, &ChartUpdater::finished, this, &ChartManager::manageThreads);
    m_threads[c->id()] = upd;
  }
  // append new charts
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
      quint32 id = r.value(0).toUInt();
      auto upd = new ChartUpdater(id, r.value(1).toString(), m_proj, viewArea, m_scale);
      connect(upd, &ChartUpdater::finished, this, &ChartManager::manageThreads);
      m_threads[id] = upd;
    }
  }
  for (auto thread: m_threads.values()) {
    thread->start();
  }

  if (noCharts && m_hadCharts) emit idle();
}

bool ChartManager::isValidScale(const Camera* cam, quint32 scale) const {

  if (m_charts.isEmpty()) return true;

  const QRectF vp = cam->boundingBox().translated(m_proj->fromWGS84(cam->geoprojection()->reference()));

  for (const LocationArea& a: m_locationAreas) {
    const WGS84Point ref = a.charts.first().ref;
    const float d = (m_proj->reference() - ref).meters();
    if (d > maxRadius) continue; // this test removes dateline and similar problems
    if (!vp.intersects(a.bbox.translated(m_proj->fromWGS84(ref)))) continue;
    for (const ChartInfo& chart: a.charts) {
      if (!vp.intersects(chart.bbox.translated(m_proj->fromWGS84(chart.ref))))  continue;
      if (float(chart.scale) / scale > maxScaleRatio) continue;
      if (float(scale) / chart.scale > maxScaleRatio) continue;
      return true;
    }
  }

  return false;
}

void ChartManager::updateDB() {

  QStringList locs;
  for (const QString& loc: QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation)) {
    locs << QString("%1/qopencpn/charts").arg(loc);
  }

  QVector<quint32> oldCharts;
  QSqlQuery r = m_db.exec("select id from charts");
  while (r.next()) {
    oldCharts.append(r.value(0).toUInt());
  }

  if (!m_db.transaction()) {
    qWarning() << "Cannot create db transaction, not updating";
    return;
  }

  SimpleMercator p;
  for (const QString& loc: locs) {
    QDirIterator it(loc, QStringList() << "*.S57", QDir::Files | QDir::Readable,
                    QDirIterator::FollowSymlinks | QDirIterator::Subdirectories);
    while (it.hasNext()) {
      try {
        const QString path = it.next();
        S57ChartOutline ch(path);
        QSqlQuery r = m_db.prepare("select id, published, modified from charts where path=?");
        r.bindValue(0, QVariant::fromValue(path));
        m_db.exec(r);
        if (!r.first()) {
          // insert
          QSqlQuery s = m_db.prepare("insert into charts"
                                     "(scale, geoproj, reference, width, height, published, modified, path)"
                                     "values(?, 'SimpleMercator', ?, ?, ?, ?, ?, ?)");
          // scale (0)
          s.bindValue(0, QVariant::fromValue(ch.scale()));
          // reference, width, height (1, 2, 3)
          p.setReference(ch.reference());
          auto bb = findBoundingBox(&p, ch.extent().corners());
          auto r0 = p.toWGS84(bb.center());
          s.bindValue(1, QVariant::fromValue(r0.toISO6709()));
          s.bindValue(2, QVariant::fromValue(bb.width()));
          s.bindValue(3, QVariant::fromValue(bb.height()));
          // published, modified, path (4, 5)
          s.bindValue(4, QVariant::fromValue(ch.published().toJulianDay()));
          s.bindValue(5, QVariant::fromValue(ch.modified().toJulianDay()));
          // path (6)
          s.bindValue(6, QVariant::fromValue(path));

          m_db.exec(s);
        } else {
          oldCharts.removeOne(r.value(0).toInt());
          if (ch.published().toJulianDay() == r.value(1).toInt() &&
              ch.modified().toJulianDay() == r.value(2).toInt()) {
            continue;
          }
          // update
          QSqlQuery s = m_db.prepare("update charts set "
                                     "scale=?, reference=?, width=?, height=?, published=?, modified=? "
                                     "where id=?");
          // scale (0)
          s.bindValue(0, QVariant::fromValue(ch.scale()));
          // reference, width, height (1, 2, 3)
          p.setReference(ch.reference());
          auto bb = findBoundingBox(&p, ch.extent().corners());
          auto r0 = p.toWGS84(bb.center());
          s.bindValue(1, QVariant::fromValue(r0.toISO6709()));
          s.bindValue(2, QVariant::fromValue(bb.width()));
          s.bindValue(3, QVariant::fromValue(bb.height()));
          // published, modified, path (4, 5)
          s.bindValue(4, QVariant::fromValue(ch.published().toJulianDay()));
          s.bindValue(5, QVariant::fromValue(ch.modified().toJulianDay()));
          // id (6)
          s.bindValue(6, r.value(0));

          m_db.exec(s);
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

void ChartManager::manageThreads() {
  qDebug() << "chartmanager: manageThreads";
  auto thread = qobject_cast<ChartUpdater*>(sender());
  quint32 id = thread->chart()->id();

  if (!m_ids.contains(id)) {
    m_ids[id] = m_charts.size();
    m_charts.append(thread->chart());
  }

  delete m_threads[id];
  m_threads.remove(id);

  if (m_threads.isEmpty()) {
    if (!m_hadCharts) {
      emit active();
    } else if (m_onlyObjects) {
      emit objectsUpdated();
    } else {
      emit chartsUpdated();
    }
  }
}


ChartUpdater::ChartUpdater(S57Chart* chart, const QRectF& viewArea, quint32 scale)
  : QThread()
  , m_chart(chart)
  , m_id(0)
  , m_path()
  , m_proj(nullptr)
  , m_area(viewArea)
  , m_scale(scale) {}

ChartUpdater::ChartUpdater(quint32 id, const QString& path, const GeoProjection* p, const QRectF& viewArea, quint32 scale)
  : QThread()
  , m_chart(nullptr)
  , m_id(id)
  , m_path(path)
  , m_proj(p)
  , m_area(viewArea)
  , m_scale(scale) {}


void ChartUpdater::run() {
  if (!m_chart) {
    m_chart = new S57Chart(m_id, m_path, m_proj);
    m_area.translate(m_chart->geoProjection()->fromWGS84(m_proj->reference()));
  }
  qDebug() << "run: update paintdata";
  m_chart->updatePaintData(m_area, m_scale);
}
