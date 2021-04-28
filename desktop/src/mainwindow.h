/* -*- coding: utf-8-unix -*-
 *
 * File: desktop/src/mainwindow.h
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

#include <KXmlGuiWindow>
#include <QObject>


class QActionGroup;

class GLWindow;
class UpdaterInterface;

namespace KV {
class PreferencesDialog;
}


class MainWindow: public KXmlGuiWindow {
  Q_OBJECT

public:

  MainWindow();

protected:

  void closeEvent(QCloseEvent *event) override;

private slots:

  void on_quit_triggered();
  void on_northUp_triggered();
  void on_fullScreen_toggled(bool on);
  void on_panNorth_triggered();
  void on_panEast_triggered();
  void on_panSouth_triggered();
  void on_panWest_triggered();
  void on_rotateCW_triggered();
  void on_rotateCCW_triggered();
  void on_zoomIn_triggered();
  void on_zoomOut_triggered();
  void on_preferences_triggered();
  void on_checkCharts_triggered();

  void updateChartSets();

private:

  void readSettings();
  void writeSettings();
  void addActions();
  QString currentChartSet() const;
  QActionGroup* chartSetGroup();

private:

  GLWindow* m_GLWindow;
  QRect m_fallbackGeom;
  KV::PreferencesDialog* m_preferences;
  UpdaterInterface* m_updater;

};

