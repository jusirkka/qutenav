/* -*- coding: utf-8-unix -*-
 *
 * File: mobile/src/conf_mainwindow.h
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

#include "configgroup.h"
#include <QSizeF>

namespace Conf {

class MainWindow : public ConfigGroup {
public:

  static MainWindow *self();
  ~MainWindow();

  CONF_DECL(Chartset, chartset, QString, toString)
  CONF_DECL(WindowGeom, window_geom, QSizeF, toSizeF)
  CONF_DECL(LastGeom, last_geom, QSizeF, toSizeF)
  CONF_DECL(FullScreen, full_screen, bool, toBool)
  CONF_DECL(CacheSize, cache_size, quint32, toUInt)

  static void setChartFolders(const QStringList& v) {
    self()->m_chartFolders = v;
    QVariantList items;
    for (auto i: v) items.append(i);

    self()->m_values["chart_folders"] = items;
  }

  static QStringList ChartFolders() {
    return self()->m_chartFolders;
  }

private:

  MainWindow();

  QStringList m_chartFolders;

};

}


