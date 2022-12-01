/* -*- coding: utf-8-unix -*-
 *
 * File: dbupdater/src/state.h
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
#pragma once

#include <QState>
#include "chartdatabase.h"
#include <QMap>
#include <QHash>
#include "s57chartoutline.h"

class ChartFileReader;
class ChartFileReaderFactory;
class QThread;


namespace State {

class Worker;

class Busy: public QState
{
  Q_OBJECT

public:

  Busy();

  ~Busy();

protected:

  void onEntry(QEvent *event) override;
  void onExit(QEvent *event) override;

signals:

  void ready(bool clearCache);
  void status(const QString& msg);
  void jobsDone();


private slots:

  void outlineCreated(const QString& path, bool status, const S57ChartOutline& outline);

private:

  static const int statusFrequency = 50;
  static const int numberOfThreads = 5;


  void loadPlugins();
  void createThreads();

  using ThreadVector = QVector<QThread*>;

  using IdSet = QSet<quint32>;

  using FactoryMap = QMap<QString, ChartFileReaderFactory*>;
  using ReaderMap = QMap<QString, ChartFileReader*>;
  using ScaleMap = QMap<quint32, quint32>; // scale -> scale_id
  using ScaleChartsetMap = QMap<QString, ScaleMap>;
  using ChartsetMap = QMap<QString, quint32>; // chartset name -> chartset id

  using WorkerVector = QVector<Worker*>;

  using ReaderHash = QHash<QString, ChartFileReader*>;
  using OutlineHash = QHash<QString, S57ChartOutline>;
  using IdHash = QHash<QString, quint32>;

  void manageCharts(const QStringList& paths);

  void insertCharts();
  void updateCharts();
  void checkChartsets();
  void deleteCharts(const IdSet& ids);
  void deleteFrom(const QString& chartName, const IdSet& ids);
  void insert(const QString& path, const S57ChartOutline& ch, quint32 scale_id);
  void update(quint32 id, const S57ChartOutline& ch);
  void insertCov(quint32 chart_id, quint32 type_id, const WGS84Polygon& r);
  void cleanupDB();
  void updateScalePriorities();

  ChartDatabase m_db;

  ReaderMap m_readers;
  FactoryMap m_factories;

  ThreadVector m_threads;
  WorkerVector m_workers;


  bool m_clearCache;
  bool m_fullSync;
  IdHash m_currentCharts;
  ReaderHash m_chartReaders;
  ReaderHash::iterator m_currentReader;
  OutlineHash m_newOutlines;
  OutlineHash m_chartOutlines;
  int m_jobCount;
  int m_updateFailCount;
  int m_insertFailCount;
};

struct OutlineData {

  OutlineData(const QString& p, const ChartFileReader* r)
    : path(p)
    , reader(r) {}

  const QString path;
  const ChartFileReader* reader;

  OutlineData() = default;
  OutlineData(const OutlineData&) = default;
  OutlineData& operator= (const OutlineData&) = default;

};

class Worker: public QObject {

  Q_OBJECT

public:

  Worker(quint32 id): QObject(), m_id(id) {}

  quint32 id() const {return m_id;}

public slots:

  void createOutline(const State::OutlineData& data);

signals:

  void done(const QString& path, bool status, const S57ChartOutline& outline);

private:

  Worker(const Worker&) = delete;
  Worker& operator=(const Worker&) = delete;

  quint32 m_id;

};

}

Q_DECLARE_METATYPE(State::OutlineData)
