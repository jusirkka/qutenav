/* -*- coding: utf-8-unix -*-
 *
 * File: dbupdater/src/state.cpp
 *
 * Copyright (C) 2022 Jukka Sirkka
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

#include "state.h"
#include <QPluginLoader>
#include "chartfilereader.h"
#include <QLibraryInfo>
#include <QDir>
#include <QDirIterator>
#include <QStandardPaths>
#include <QScopedPointer>
#include <QStateMachine>
#include <QThread>
#include <QDebug>
#include "platform.h"

State::Busy::Busy()
  : QState()
  , m_db("State::Busy")
{
  loadPlugins();
  createThreads();
}

State::Busy::~Busy() {
  for (QThread* thread: m_threads) {
    thread->quit();
    thread->wait();
  }
  qDeleteAll(m_threads);
  qDeleteAll(m_readers);
}

void State::Busy::createThreads() {
  for (int i = 0; i < Platform::number_of_chart_threads(); ++i) {
    qDebug() << "creating thread" << i;
    auto thread = new QThread;
    qDebug() << "creating worker" << i;
    auto worker = new Worker(m_workers.size());
    connect(thread, &QThread::finished, worker, &QObject::deleteLater);
    connect(worker, &State::Worker::done, this, &State::Busy::outlineCreated);
    qDebug() << "moving worker to thread" << i;
    worker->moveToThread(thread);
    qDebug() << "starting thread" << i;
    thread->start();
    m_threads.append(thread);
    m_workers.append(worker);
  }
  qDebug() << "threads started";
}

void State::Busy::onEntry(QEvent* event) {
  if (event->type() == QEvent::StateMachineSignal) {
    auto ev = static_cast<QStateMachine::SignalEvent*>(event);

    auto paths = ev->arguments()[0].toStringList();

    m_clearCache = false;
    m_fullSync = ev->arguments()[1].toBool();

    manageCharts(paths);
  }
  QState::onEntry(event);
}

void State::Busy::manageCharts(const QStringList& dirs) {
  // traverse paths -> candidates
  ReaderHash candidates;
  m_currentCharts.clear();
  for (auto ft = m_factories.cbegin(); ft != m_factories.cend(); ++ft) {
    const ChartFileReaderFactory* ftor = ft.value();
    for (const auto& dir: dirs) {
      QDirIterator it(dir,
                      ftor->filters(),
                      QDir::Files | QDir::Readable,
                      QDirIterator::FollowSymlinks | QDirIterator::Subdirectories);
      if (!it.hasNext()) continue;
      if (!m_readers.contains(ftor->name())) {
        auto reader = ftor->loadReader(dirs);
        if (reader == nullptr) continue;
        m_readers[ftor->name()] = reader;
      }
      auto reader = m_readers[ftor->name()];
      while (it.hasNext()) {
        candidates[it.next()] = reader;
      }
    }
  }
  IdSet unwanted;
  // fetch current charts
  QSqlQuery r = m_db.exec("select id, path from charts");
  while (r.next()) {
    const quint32 id = r.value(0).toUInt();
    const auto path = r.value(1).toString();
    if (candidates.contains(path)) {
      if (m_fullSync) {
        m_currentCharts[path] = id;
      } else {
        candidates.remove(path);
      }
    } else {
      unwanted.insert(id);
    }
  }
  // check chartsets
  checkChartsets();
  // remove the remove set
  deleteCharts(unwanted);

  // initiate outline fetch
  m_chartOutlines.clear();
  m_newOutlines.clear();
  m_chartReaders = candidates;
  m_currentReader = m_chartReaders.begin();
  m_jobEndCount = 0;
  m_updateFailCount = 0;
  m_insertFailCount = 0;
  for (Worker* worker: m_workers) {
    if (m_currentReader == m_chartReaders.end()) break;
    const QString path = m_currentReader.key();
    const ChartFileReader* reader = m_currentReader.value();
    QMetaObject::invokeMethod(worker, "createOutline",
                              Q_ARG(const State::OutlineData&, OutlineData(path, reader)));
    ++m_currentReader;
  }
}


void State::Busy::outlineCreated(const QString& path,
                                 bool ok,
                                 const S57ChartOutline& outline) {
  if (!active()) return;

  m_jobEndCount += 1;

  if (ok) {
    if (m_currentCharts.contains(path)) {
      m_chartOutlines[path] = outline;
      if (m_chartOutlines.size() % statusFrequency == 0) {
        emit status(QString("Update:%1:%2")
                    .arg(m_chartOutlines.size())
                    .arg(m_currentCharts.size() - m_updateFailCount));
      }
    } else {
      m_newOutlines[path] = outline;
      if (m_newOutlines.size() % statusFrequency == 0) {
        emit status(QString("Insert:%1:%2")
                    .arg(m_newOutlines.size())
                    .arg(m_chartReaders.size() - m_currentCharts.size() - m_insertFailCount));
      }
    }
  } else {
    if (m_currentCharts.contains(path)) {
      m_updateFailCount += 1;
    } else {
      m_insertFailCount += 1;
    }
  }

  if (m_jobEndCount == m_chartReaders.size()) {
    if (m_chartOutlines.size() % statusFrequency != 0) {
      emit status(QString("Update:%1:%1").arg(m_chartOutlines.size()));
    }
    if (m_newOutlines.size() % statusFrequency != 0) {
      emit status(QString("Insert:%1:%1").arg(m_newOutlines.size()));
    }
    emit jobsDone();
  } else if (m_currentReader != m_chartReaders.end()) {
    const QString path = m_currentReader.key();
    const ChartFileReader* reader = m_currentReader.value();
    QMetaObject::invokeMethod(sender(), "createOutline",
                              Q_ARG(const State::OutlineData&, OutlineData(path, reader)));
    ++m_currentReader;
  }
}


void State::Busy::onExit(QEvent* event) {
  if (event->type() == QEvent::StateMachineSignal) {
    auto ev = static_cast<QStateMachine::SignalEvent*>(event);

    if (ev->signalIndex() ==
        ev->sender()->metaObject()->indexOfSignal("jobsDone()")) {
      // insert new charts
      insertCharts();
      // delete empty chartsets & unused scales
      cleanupDB();
      // update current charts
      updateCharts();
      updateScalePriorities();
      if (m_fullSync) {
        emit status("Ready:FullSync");
      } else {
        emit status("Ready:Sync");
      }
      qDebug() << "emit ready, clearCache =" << m_clearCache;
      emit ready(m_clearCache);
    }
    m_currentCharts.clear();
    m_chartReaders.clear();
    m_newOutlines.clear();
    m_chartOutlines.clear();
  }
  QState::onExit(event);
}


void State::Busy::checkChartsets() {
  QStringList chartSets(m_readers.keys());

  // remove known chartsets
  QSqlQuery s = m_db.exec("select name from chartsets");
  while (s.next()) chartSets.removeOne(s.value(0).toString());

  // remaining are the new ones
  for (const auto& chartSet: chartSets) {
    // insert
    QSqlQuery r = m_db.prepare("insert into chartsets (name) values (?)");
    r.bindValue(0, chartSet);
    m_db.exec(r);
  }
}

void State::Busy::deleteCharts(const IdSet& c_ids) {

  if (c_ids.isEmpty()) return;

  m_clearCache = true;

  const QString sql = QString("select p.id, v.id from "
                              "polygons p join "
                              "coverage v on p.cov_id = v.id "
                              "where v.chart_id in (?%1)")
      .arg(QString(",?").repeated(c_ids.size() - 1));

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

  deleteFrom("charts", c_ids);
  deleteFrom("coverage", v_ids);
  deleteFrom("polygons", p_ids);
}


void State::Busy::deleteFrom(const QString& chartName, const IdSet& ids) {
  if (ids.isEmpty()) return;

  int start = 0;
  int chunk = std::min(10000, ids.size());
  const auto idList = ids.values();

  while (start < ids.size()) {
    if (chunk <= 0) break;
    const auto sql = QString("delete from %1 where id in (?%2)")
        .arg(chartName)
        .arg(QString(",?").repeated(chunk - 1));
    QSqlQuery r0 = m_db.prepare(sql);
    for (int index = 0; index < chunk; index++) {
      r0.bindValue(index, QVariant::fromValue(idList[start + index]));
    }
    m_db.exec(r0);
    start += chunk;
    chunk = std::min(10000, ids.size() - start);
  }
}

void State::Busy::insertCharts() {

  ChartsetMap chartsets;
  QSqlQuery r1 = m_db.exec("select name, id from chartsets");
  while (r1.next()) {
    chartsets[r1.value(0).toString()] = r1.value(1).toUInt();
  }

  ScaleChartsetMap scales;
  QSqlQuery r = m_db.exec("select c.name, s.scale, s.id "
                          "from scales s join "
                          "chartsets c on c.id = s.chartset_id");
  while (r.next()) {
    scales[r.value(0).toString()][r.value(1).toUInt()] = r.value(2).toUInt();
  }

  if (!m_db.transaction()) {
    qWarning() << "Cannot create db transaction, not inserting new charts";
    return;
  }

  for (auto it = m_newOutlines.cbegin(); it != m_newOutlines.cend(); ++it) {
    const auto ch = it.value();
    const auto path = it.key();
    const auto name = m_chartReaders[path]->name();
    if (!scales.contains(name) || !scales[name].contains(ch.scale())) {
      auto r4 = m_db.prepare("insert into scales "
                             "(chartset_id, scale, priority) "
                             "values(?, ?, 0)");
      r4.bindValue(0, chartsets[name]);
      r4.bindValue(1, ch.scale());
      m_db.exec(r4);
      scales[name][ch.scale()] = r4.lastInsertId().toUInt();
    }

    insert(path, ch, scales[name][ch.scale()]);
  }

  if (!m_db.commit()) {
    qWarning() << "DB commit failed!";
  }
}

void State::Busy::insert(const QString& path, const S57ChartOutline& ch, quint32 scale_id) {
  // insert into charts
  QSqlQuery t = m_db.prepare("insert into charts "
                             "(scale_id, swx, swy, nex, ney, "
                             "published, modified, path) "
                             "values(?, ?, ?, ?, ?, ?, ?, ?)");
  // scale_id (0)
  t.bindValue(0, scale_id);
  // sw, ne (1, 2, 3, 4)
  auto sw = ch.extent().sw();
  auto ne = ch.extent().ne();
  t.bindValue(1, sw.lng());
  t.bindValue(2, sw.lat());
  t.bindValue(3, ne.lng(sw));
  t.bindValue(4, ne.lat());
  // published, modified, path (5, 6, 7)
  t.bindValue(5, ch.published().toJulianDay());
  t.bindValue(6, ch.modified().toJulianDay());
  t.bindValue(7, path);

  m_db.exec(t);
  uint chart_id = t.lastInsertId().toUInt();

  insertCov(chart_id, 1, ch.coverage());
  insertCov(chart_id, 2, ch.nocoverage());
}

void State::Busy::insertCov(quint32 chart_id, quint32 type_id,
                            const WGS84Polygon &ps) {
  // insert into coverage
  QVector<quint32> ids;
  int cnt = ps.size();
  while (cnt > 0) {
    QSqlQuery r = m_db.prepare("insert into coverage "
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
    for (const WGS84Point& p: ps[k]) {
      QSqlQuery r = m_db.prepare("insert into polygons "
                                 "(cov_id, x, y) "
                                 "values(?, ?, ?)");
      r.bindValue(0, ids[k]);
      r.bindValue(1, p.lng());
      r.bindValue(2, p.lat());
      m_db.exec(r);
    }
  }
}

void State::Busy::cleanupDB() {
  // delete empty chartsets
  IdSet set_ids;
  QSqlQuery r1 = m_db.exec("select id from chartsets");
  while (r1.next()) set_ids << r1.value(0).toUInt();

  IdSet s_ids;
  for (auto set_id: set_ids) {
    QSqlQuery r2 = m_db.prepare("select c.id from "
                                "charts c join "
                                "scales s on c.scale_id = s.id "
                                "where s.chartset_id = ? limit 1");
    r2.bindValue(0, set_id);
    m_db.exec(r2);
    if (!r2.first()) {
      s_ids << set_id;
    }
  }

  deleteFrom("chartsets", s_ids);

  // delete unused scales
  IdSet scale_ids;
  QSqlQuery r3 = m_db.exec("select id from scales");
  while (r3.next()) scale_ids << r3.value(0).toUInt();

  IdSet sc_ids;
  for (auto scale_id: scale_ids) {
    QSqlQuery r4 = m_db.prepare("select id from charts "
                                "where scale_id = ? limit 1");
    r4.bindValue(0, scale_id);
    m_db.exec(r4);
    if (!r4.first()) {
      sc_ids << scale_id;
    }
  }

  deleteFrom("scales", sc_ids);

}

void State::Busy::updateCharts() {
  for (auto it = m_chartOutlines.cbegin(); it != m_chartOutlines.cend(); ++it) {
    const auto ch = it.value();
    const auto id = m_currentCharts[it.key()];
    QSqlQuery s = m_db.prepare("select published, modified "
                               "from charts where id = ?");
    s.bindValue(0, id);
    m_db.exec(s);

    if (!s.first() || (ch.published().toJulianDay() == s.value(0).toInt() &&
                       ch.modified().toJulianDay() == s.value(1).toInt())) {
      continue;
    }

    update(id, ch);
  }
}

void State::Busy::update(quint32 id, const S57ChartOutline &ch) {

  m_clearCache = true;

  // update charts
  QSqlQuery t = m_db.prepare("update charts set "
                             "swx=?, swy=?, nex=?, ney=?, "
                             "published=?, modified=? "
                             "where id=?");
  // sw, ne (0, 1, 2, 3)
  t.bindValue(0, ch.extent().sw().lng());
  t.bindValue(1, ch.extent().sw().lat());
  t.bindValue(2, ch.extent().ne().lng());
  t.bindValue(3, ch.extent().ne().lat());
  // published, modified, path (4, 5)
  t.bindValue(4, ch.published().toJulianDay());
  t.bindValue(5, ch.modified().toJulianDay());
  // id (6)
  t.bindValue(6, id);

  m_db.exec(t);

  // delete old cov & polygon rows
  IdSet cov;
  QSqlQuery r0 = m_db.prepare("select id "
                              "from coverage "
                              "where chart_id=?");
  r0.bindValue(0, id);
  m_db.exec(r0);
  while (r0.next()) {
    cov << r0.value(0).toUInt();
  }
  deleteFrom("polygons", cov);

  QSqlQuery r2 = m_db.prepare("delete from coverage "
                              "where chart_id=?");
  r2.bindValue(0, id);
  m_db.exec(r2);

  // insert new coverage regions
  insertCov(id, 1, ch.coverage());
  insertCov(id, 2, ch.nocoverage());
}

void State::Busy::updateScalePriorities() {
  QMap<quint32, QString> chartSets;

  using ScaleVector = QVector<quint32>;

  QSqlQuery r0 = m_db.exec("select id, name from chartsets");
  while (r0.next()) chartSets[r0.value(0).toUInt()] = r0.value(1).toString();

  for (auto it = chartSets.cbegin(); it != chartSets.cend(); ++it) {
    qDebug() << "Updating" << it.value() << "priorities";

    QSqlQuery r1 = m_db.prepare("select scale from scales where chartset_id=?");
    r1.bindValue(0, it.key());
    m_db.exec(r1);
    ScaleVector scales;
    while (r1.next()) scales.append(r1.value(0).toUInt());

    std::sort(scales.begin(), scales.end(), [] (quint32 s1, quint32 s2) {
      return s1 > s2;
    });

    if (!m_db.transaction()) {
      qWarning() << "Cannot create db transaction";
    }

    quint32 cnt = 1;
    for (auto s: scales) {
      QSqlQuery r2 = m_db.prepare("update scales set priority=? where scale=? and chartset_id=?");
      r2.bindValue(0, cnt);
      r2.bindValue(1, s);
      r2.bindValue(2, it.key());
      m_db.exec(r2);
      cnt++;
    }
    if (!m_db.commit()) {
      qWarning() << "DB commit failed!";
    }
  }
}

void State::Busy::loadPlugins() {
  const auto& staticFactories = QPluginLoader::staticInstances();
  for (auto plugin: staticFactories) {
    auto factory = qobject_cast<ChartFileReaderFactory*>(plugin);
    if (!factory) continue;
    qDebug() << "Loaded reader plugin" << factory->name();
    m_factories[factory->name()] = factory;
  }

  QString base = QLibraryInfo::location(QLibraryInfo::PluginsPath);
  QDir pluginsDir(QString("%1/qutenav").arg(base));
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

void State::Worker::createOutline(const OutlineData& d) {
  S57ChartOutline ch;
  bool ok = true;
  try {
    auto gp = QScopedPointer<GeoProjection>(d.reader->configuredProjection(d.path));
    ch = d.reader->readOutline(d.path, gp.data());
  } catch (ChartFileError& e) {
    qWarning() << "Chart file error in" << d.path << ":" << e.msg() << ", skipping";
    ok = false;
  }
  emit done(d.path, ok, ch);
}

