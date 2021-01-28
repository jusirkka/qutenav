#include "updater.h"
#include <QDebug>
#include <QPluginLoader>
#include "chartfilereader.h"
#include <QLibraryInfo>
#include <QDir>
#include <QDirIterator>
#include <QStandardPaths>
#include <QFileSystemWatcher>


Updater::Updater(QObject* parent)
  : QObject(parent)
  , m_db()
  , m_watcher(new QFileSystemWatcher)
  , m_chartDirs()
  , m_idle(true)
{
  loadPlugins();
  sync();
  connect(m_watcher, &QFileSystemWatcher::directoryChanged, this, [this] (const QString& dir) {
    checkChartsDir(dir, true);
  });
}

Updater::~Updater() {
  qDeleteAll(m_readers);
  delete m_watcher;
}

void Updater::sync() {
  checkChartDirs();
  for (auto& path: m_chartDirs) {
    checkChartsDir(path, false);
  }
  emit ready();
}

void Updater::loadPlugins() {
  const auto& staticFactories = QPluginLoader::staticInstances();
  for (auto plugin: staticFactories) {
    auto factory = qobject_cast<ChartFileReaderFactory*>(plugin);
    if (!factory) continue;
    qDebug() << "Loaded reader plugin" << factory->name();
    m_factories[factory->name()] = factory;
  }

  QString base = QLibraryInfo::location(QLibraryInfo::PluginsPath);
  QDir pluginsDir(QString("%1/qopencpn").arg(base));
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

void Updater::checkChartDirs() {


  // qopencpn or harbour-qopencpn
  const QString baseapp = qAppName().split("_").first();
  QStringList locs;
  for (const QString& loc: QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation)) {
    locs << QString("%1/%2/charts").arg(loc).arg(baseapp);
  }
  qDebug() << locs;

  QSet<QString> readers;
  QStringList presentChartDirs = m_chartDirs;

  for (const QString& loc: locs) {
    QDir dataDir(loc);
    const QStringList dirs = dataDir.entryList(QStringList(),
                                               QDir::Dirs | QDir::Readable);
    for (auto dir: dirs) {
      if (m_factories.contains(dir)) {
        readers << dir;
        auto path = dataDir.absoluteFilePath(dir);
        if (!m_chartDirs.contains(path)) {
          m_chartDirs << path;
          m_watcher->addPath(path);
        } else if (presentChartDirs.contains(path)) {
          presentChartDirs.removeAll(path);
        }
      }
    }
  }

  for (auto& path: presentChartDirs) {
    m_watcher->removePath(path);
  }

  for (auto name: readers) {
    qDebug() << "loading reader" << name;
    m_readers[name] = m_factories[name]->loadReader();
  }

  QSqlQuery s = m_db.exec("select name "
                          "from main.chartsets");
  while (s.next()) {
    readers.remove(s.value(0).toString());
  }

  for (auto& name: readers) {
    // insert
    QSqlQuery r = m_db.prepare("insert into main.chartsets (name) values(?)");
    r.bindValue(0, name);
    m_db.exec(r);
  }
}


void Updater::checkChartsDir(const QString& dir, bool notify) {

  if (!m_chartDirs.contains(dir)) return;

  const QString name = QFileInfo(dir).baseName();
  qDebug() << "checkChartsDir" << dir << name;

  QSqlQuery r1 = m_db.prepare("select id "
                              "from main.chartsets "
                              "where name=?");
  r1.bindValue(0, QVariant::fromValue(name));
  m_db.exec(r1);
  r1.first();
  uint set_id = r1.value(0).toUInt();


  QVector<quint32> oldCharts;
  QSqlQuery r2 = m_db.prepare("select c.id "
                              "from main.charts c "
                              "join main.scales s on c.scale_id = s.id "
                              "where s.chartset_id=?");
  r2.bindValue(0, set_id);
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

  QDirIterator it(dir,
                  m_factories[name]->filters(),
                  QDir::Files | QDir::Readable,
                  QDirIterator::FollowSymlinks | QDirIterator::Subdirectories);
  while (it.hasNext()) {
    const QString path = it.next();
    try {
      // qDebug() << "reading outline" << path;
      S57ChartOutline ch = m_readers[name]->readOutline(path);
      if (!scales.contains(ch.scale())) {
        auto r4 = m_db.prepare("insert into main.scales"
                               "(chartset_id, scale) "
                               "values(?, ?)");
        r4.bindValue(0, set_id);
        r4.bindValue(1, ch.scale());
        m_db.exec(r4);
        scales[ch.scale()] = r4.lastInsertId().toUInt();
      }

      QSqlQuery s = m_db.prepare("select id, published, modified "
                                 "from main.charts where path=?");
      s.bindValue(0, path);
      m_db.exec(s);

      if (!s.first()) {

        insert(path, ch, scales);

      } else {

        oldCharts.removeOne(s.value(0).toInt());
        if (ch.published().toJulianDay() == s.value(1).toInt() &&
            ch.modified().toJulianDay() == s.value(2).toInt()) {
          continue;
        }

        update(s.value(0), ch);

      }
    } catch (ChartFileError& e) {
      qWarning() << "Chart file error in" << path << ":" << e.msg() << ", skipping";
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

  if (notify) {
    emit ready();
  }
}

void Updater::insert(const QString &path, const S57ChartOutline &ch, const ScaleMap& scales) {
  // insert into charts
  QSqlQuery t = m_db.prepare("insert into main.charts "
                             "(scale_id, swx, swy, nex, ney, "
                             "published, modified, path) "
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
  uint chart_id = t.lastInsertId().toUInt();

  insertCov(chart_id, 1, ch.coverage());
  insertCov(chart_id, 2, ch.nocoverage());
}

void Updater::insertCov(quint32 chart_id, quint32 type_id,
                        const S57ChartOutline::Region &r) {
  // insert into coverage
  QVector<quint32> ids;
  int cnt = r.size();
  while (cnt > 0) {
    QSqlQuery r = m_db.prepare("insert into main.coverage "
                               "(type_id, chart_id) "
                               "values(?, ?)");
    r.bindValue(0, type_id);
    r.bindValue(1, chart_id);
    m_db.exec(r);
    ids << r.lastInsertId().toUInt();
    cnt--;
  }
  // insert into polygons
  for (int k = 0; k < ids.size(); k++) {
    for (const WGS84Point& p: r[k]) {
      QSqlQuery r = m_db.prepare("insert into main.polygons "
                                 "(cov_id, x, y) "
                                 "values(?, ?, ?)");
      r.bindValue(0, ids[k]);
      r.bindValue(1, p.lng());
      r.bindValue(2, p.lat());
      m_db.exec(r);
    }
  }
}

void Updater::update(const QVariant &id, const S57ChartOutline &ch) {
  // update charts
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
  t.bindValue(6, id);

  m_db.exec(t);

  // delete old cov & polygon rows
  QVector<quint32> cov;
  QSqlQuery r0 = m_db.prepare("select id "
                              "from main.coverage "
                              "where chart_id=?");
  r0.bindValue(0, id);
  m_db.exec(r0);
  while (r0.next()) {
    cov.append(r0.value(0).toUInt());
  }

  QString sql = "delete from main.polygons where cov_id in (";
  sql += QString("?,").repeated(cov.size());
  sql = sql.replace(sql.length() - 1, 1, ")");
  QSqlQuery r1 = m_db.prepare(sql);
  for (int i = 0; i < cov.size(); i++) {
    r1.bindValue(i, cov[i]);
  }
  m_db.exec(r1);

  QSqlQuery r2 = m_db.prepare("delete from main.coverage "
                              "where chart_id=?");
  r2.bindValue(0, id);
  m_db.exec(r2);

  // insert new coverage regions
  const uint chart_id = id.toUInt();
  insertCov(chart_id, 1, ch.coverage());
  insertCov(chart_id, 2, ch.nocoverage());

}
