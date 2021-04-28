/* -*- coding: utf-8-unix -*-
 *
 * File: dbupdater/src/updater.cpp
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
#include "updater.h"
#include <QDebug>
#include <QPluginLoader>
#include "chartfilereader.h"
#include <QLibraryInfo>
#include <QDir>
#include <QDirIterator>
#include <QStandardPaths>
#include <QScopedPointer>


Updater::Updater(QObject* parent)
  : QObject(parent)
  , m_db()
  , m_chartDirs()
{
  loadPlugins();
}

Updater::~Updater() {
  qDeleteAll(m_readers);
}

void Updater::sync() {
  checkChartDirs();

  IdSet prevCharts;
  QSqlQuery r = m_db.exec("select id from main.charts");
  while (r.next()) {
    prevCharts << r.value(0).toUInt();
  }

  for (auto& path: m_chartDirs) {
    checkChartsDir(path, prevCharts);
  }

  // remove old rows
  deleteCharts(prevCharts);

  // delete empty chartsets & unused scales
  cleanupDB();

  qDebug() << "emit ready";
  emit ready();
}

QString Updater::ping() const {
  return "pong";
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

  m_chartDirs.clear();
  QSet<QString> chartSets;

  for (const QString& loc: locs) {
    QDir dataDir(loc);
    const QStringList dirs = dataDir.entryList(QStringList(),
                                               QDir::Dirs | QDir::Readable);
    for (auto dir: dirs) {
      if (m_factories.contains(dir)) {
        if (!m_readers.contains(dir)) {
          qDebug() << "loading reader" << dir;
          m_readers[dir] = m_factories[dir]->loadReader();
        }
        chartSets << dir;
        auto path = dataDir.absoluteFilePath(dir);
        m_chartDirs << path;
      }
    }
  }

  // Insert new chartsets

  // remove known chartsets
  QSqlQuery s = m_db.exec("select name from main.chartsets");
  while (s.next()) chartSets.remove(s.value(0).toString());

  // remaining are the new ones
  for (auto& chartSet: chartSets) {
    // insert
    QSqlQuery r = m_db.prepare("insert into main.chartsets (name) values (?)");
    r.bindValue(0, chartSet);
    m_db.exec(r);
  }
}


void Updater::checkChartsDir(const QString& dir, IdSet& prevCharts) {

  const QString name = QFileInfo(dir).baseName();
  qDebug() << "checkChartsDir" << dir << name;

  QSqlQuery r1 = m_db.prepare("select id "
                              "from main.chartsets "
                              "where name=?");
  r1.bindValue(0, name);
  m_db.exec(r1);
  r1.first();
  uint set_id = r1.value(0).toUInt();

  ScaleMap scales;
  QSqlQuery r3 = m_db.prepare("select id, scale "
                              "from main.scales "
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
      auto gp = QScopedPointer<GeoProjection>(m_readers[name]->configuredProjection(path));
      // qDebug() << "reading outline" << path;
      S57ChartOutline ch = m_readers[name]->readOutline(path, gp.data());
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

        prevCharts.remove(s.value(0).toInt());
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

  if (!m_db.commit()) {
    qWarning() << "DB commit failed!";
  }
}

void Updater::deleteCharts(const IdSet& c_ids) {

  if (c_ids.isEmpty()) return;

  QString sql = "select p.id, v.id from "
                "main.polygons p join "
                "main.coverage v on p.cov_id = v.id "
                "where v.chart_id in (";
  sql += QString("?,").repeated(c_ids.size());
  sql = sql.replace(sql.length() - 1, 1, ")");
  QSqlQuery r0 = m_db.prepare(sql);
  int index = 0;
  for (auto c_id: c_ids) {
    r0.bindValue(index, QVariant::fromValue(c_id));
    index++;
  }
  m_db.exec(r0);

  IdSet v_ids;
  IdSet p_ids;

  while (r0.next()) {
    p_ids << r0.value(0).toUInt();
    v_ids << r0.value(1).toUInt();
  }

  deleteFrom("main.charts", c_ids);
  deleteFrom("main.coverage", v_ids);
  deleteFrom("main.polygons", p_ids);
}


void Updater::cleanupDB() {
  // delete empty chartsets
  IdSet set_ids;
  QSqlQuery r1 = m_db.exec("select id from main.chartsets");
  while (r1.next()) set_ids << r1.value(0).toUInt();

  IdSet s_ids;
  for (auto set_id: set_ids) {
    QSqlQuery r2 = m_db.prepare("select c.id from "
                                "main.charts c join "
                                "main.scales s on c.scale_id = s.id "
                                "where s.chartset_id = ? limit 1");
    r2.bindValue(0, set_id);
    m_db.exec(r2);
    if (!r2.first()) {
      s_ids << set_id;
    }
  }

  deleteFrom("main.chartsets", s_ids);

  // delete unused scales
  IdSet scale_ids;
  QSqlQuery r3 = m_db.exec("select id from main.scales");
  while (r3.next()) scale_ids << r3.value(0).toUInt();

  IdSet sc_ids;
  for (auto scale_id: scale_ids) {
    QSqlQuery r4 = m_db.prepare("select id from main.charts "
                                "where scale_id = ? limit 1");
    r4.bindValue(0, scale_id);
    m_db.exec(r4);
    if (!r4.first()) {
      sc_ids << scale_id;
    }
  }

  deleteFrom("main.scales", sc_ids);

}

void Updater::deleteFrom(const QString &chartName, const IdSet &ids) {
  if (ids.isEmpty()) return;

  auto sql = QString("delete from %1 where id in (").arg(chartName);
  sql += QString("?,").repeated(ids.size());
  sql = sql.replace(sql.length() - 1, 1, ")");
  QSqlQuery r0 = m_db.prepare(sql);
  int index = 0;
  for (auto id: ids) {
    r0.bindValue(index, QVariant::fromValue(id));
    index++;
  }
  m_db.exec(r0);
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
  IdSet cov;
  QSqlQuery r0 = m_db.prepare("select id "
                              "from main.coverage "
                              "where chart_id=?");
  r0.bindValue(0, id);
  m_db.exec(r0);
  while (r0.next()) {
    cov << r0.value(0).toUInt();
  }
  deleteFrom("main.polygons", cov);

  QSqlQuery r2 = m_db.prepare("delete from main.coverage "
                              "where chart_id=?");
  r2.bindValue(0, id);
  m_db.exec(r2);

  // insert new coverage regions
  const uint chart_id = id.toUInt();
  insertCov(chart_id, 1, ch.coverage());
  insertCov(chart_id, 2, ch.nocoverage());

}
