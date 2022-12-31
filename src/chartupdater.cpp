/* -*- coding: utf-8-unix -*-
 *
 * chartupdater.cpp
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

#include "chartupdater.h"
#include "s57chart.h"
#include "logging.h"
#include "platform.h"
#include "chartcover.h"
#include "cachereader.h"
#include <QFileInfo>
#include <QDir>
#include <QFile>
#include <QStandardPaths>

ChartData::ChartData(S57Chart* c,
                     quint32 s, const WGS84PointVector& cs, bool upd)
  : chart(c)
  , id(0)
  , path()
  , scale(s)
  , cover(cs)
  , updLup(upd)
{}

ChartData::ChartData(quint32 i, int p, const QString& pth,
                     quint32 s, const WGS84PointVector& cs)
  : chart(nullptr)
  , id(i)
  , priority(p)
  , path(pth)
  , scale(s)
  , cover(cs)
  , updLup(false)
{}



void ChartUpdater::createChart(const ChartData& d) {
  try {
    auto chart = new S57Chart(d.id, d.priority, d.path);
    chart->updatePaintData(d.cover, d.scale);
    emit done(chart);
  } catch (ChartFileError& e) {
    qCWarning(CMGR) << "Chart creation failed:" << e.msg();
    emit done(nullptr);
  }
}

void ChartUpdater::updateChart(const ChartData& d) {
  if (d.updLup) {
    d.chart->updateLookups();
  }
  d.chart->updatePaintData(d.cover, d.scale);
  emit done(d.chart);
}

void ChartUpdater::requestInfo(S57Chart *chart, const WGS84Point &p,
                               quint32 scale, bool full, quint32 tid) {
  auto info = full ? chart->objectInfoFull(p, scale) : chart->objectInfo(p, scale);
  emit infoResponse(info, tid);
}


void ChartUpdater::cacheChart(S57Chart *chart) {
  auto scoped = QScopedPointer<S57Chart>(chart);

  const auto id = CacheReader::CacheId(chart->path());
  const auto base = QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation);


  const auto cacheDir = QString("%1/%2").arg(base).arg(baseAppName());
  const auto cachePath = QString("%1/%2").arg(cacheDir).arg(QString(id));

  if (!QFileInfo(cachePath).exists()) {
    // not found - cache
    if (!QDir().mkpath(cacheDir)) return;
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
