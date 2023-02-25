/* -*- coding: utf-8-unix -*-
 *
 * File: mobile/src/conf_mainwindow.cpp
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
#include "conf_mainwindow.h"

Conf::MainWindow* Conf::MainWindow::self() {
  static MainWindow* s = new MainWindow();
  return s;
}

Conf::MainWindow::MainWindow()
  : ConfigGroup("MainWindow", "qutenavrc")
{
  m_defaults["chartset"] = "None";
  m_defaults["window_geom"] = QSizeF(800, 600);
  m_defaults["last_geom"] = QSizeF(800, 600);
  m_defaults["full_screen"] = false;
  m_defaults["chart_folders"] = QVariantList();
  m_defaults["shown_eulas"] = QVariantList();
  m_defaults["cache_size"] = 2000; // 2GB

  load();

  QVariantList vitems = m_values["chart_folders"].toList();
  for (auto v: vitems) m_chartFolders.append(v.toString());

  vitems = m_values["shown_eulas"].toList();
  for (auto v: vitems) m_shownEulas.append(v.toString());
}

Conf::MainWindow::~MainWindow() {/*noop*/}
