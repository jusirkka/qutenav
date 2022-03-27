/* -*- coding: utf-8-unix -*-
 *
 * File: dbupdater/src/updater.h
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
#pragma once

#include <QObject>
#include "chartdatabase.h"
#include <QMap>
#include <QHash>
#include "s57chartoutline.h"

class ChartFileReader;
class ChartFileReaderFactory;

class Updater: public QObject {

  Q_OBJECT

public:

  Updater(QObject* parent = nullptr);
  ~Updater();

public slots:

  void sync(const QStringList& paths);
  void fullSync(const QStringList& paths);
  QString ping() const;


signals:

  void ready();
  void status(const QString& msg);


private:

  static const int statusFrequency = 100;

  using IdSet = QSet<quint32>;

  using FactoryMap = QMap<QString, ChartFileReaderFactory*>;
  using ReaderMap = QMap<QString, ChartFileReader*>;
  using ScaleMap = QMap<quint32, quint32>; // scale -> scale_id
  using ScaleChartsetMap = QMap<QString, ScaleMap>;
  using ChartsetMap = QMap<QString, quint32>; // chartset name -> chartset id

  struct ChartInfo {
    ChartInfo() = default;
    ChartInfo(const QString& p, ChartFileReader* r)
      : path(p)
      , reader(r) {}
    QString path;
    ChartFileReader* reader;
  };

  using ChartInfoMap = QMap<quint32, ChartInfo>;
  using ChartInfoVector = QVector<ChartInfo>;
  using PathHash = QHash<QString, ChartFileReader*>;

  void manageCharts(const QStringList& paths, ChartInfoMap* charts);
  void updateCharts(const ChartInfoMap& charts);

  void insertCharts(const PathHash& paths);
  void checkChartsets();
  void loadPlugins();
  void deleteCharts(const IdSet& ids);
  void deleteFrom(const QString& chartName, const IdSet& ids);
  void insert(const QString& path, const S57ChartOutline& ch, quint32 scale_id);
  void update(quint32 id, const S57ChartOutline& ch);
  void insertCov(quint32 chart_id, quint32 type_id, const WGS84Polygon& r);
  void cleanupDB();

  ChartDatabase m_db;

  ReaderMap m_readers;
  FactoryMap m_factories;
};
