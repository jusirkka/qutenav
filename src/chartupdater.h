/* -*- coding: utf-8-unix -*-
 *
 * chartupdater.h
 *
 * Created: 2021-03-18 2021 by Jukka Sirkka
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
#include "region.h"
#include "types.h"
#include "geoprojection.h"

class S57Chart;

struct ChartData {

  ChartData(S57Chart* c,
            quint32 s, const WGS84PointVector& cover, bool upd);

  ChartData(quint32 i, const QString& pth,
            quint32 s, const WGS84PointVector& cover);

  S57Chart* chart;
  quint32 id;
  QString path;
  quint32 scale;
  WGS84PointVector cover;
  bool updLup;

  ChartData() = default;
  ChartData(const ChartData&) = default;
  ChartData& operator= (const ChartData&) = default;
};

class ChartUpdater: public QObject {

  Q_OBJECT

public:

  ChartUpdater(quint32 id): QObject(), m_id(id) {}

  quint32 id() const {return m_id;}



public slots:

  void updateChart(const ChartData& d);
  void createChart(const ChartData& d);
  void cacheChart(S57Chart* chart);
  void requestInfo(S57Chart* chart, const WGS84Point& p, quint32 scale, quint32 tid);

signals:

  void done(S57Chart* chart);
  void infoResponse(const S57::InfoType& info, quint32 tid);

private:

  ChartUpdater(const ChartUpdater&) = delete;
  ChartUpdater& operator=(const ChartUpdater&) = delete;

  quint32 m_id;

};

Q_DECLARE_METATYPE(ChartData)


