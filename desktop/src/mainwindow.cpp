/* -*- coding: utf-8-unix -*-
 *
 * File: desktop/src/mainwindow.cpp
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
#include "mainwindow.h"
#include "glwindow.h"
#include "globe.h"
#include <KActionCollection>
#include <KHelpMenu>
#include <KAboutData>
#include <QApplication>
#include "conf_mainwindow.h"
#include "conf_marinerparams.h"
#include <QCloseEvent>
#include <QStatusBar>
#include <QMenu>
#include "chartmanager.h"
#include <QMenuBar>
#include "preferences.h"
#include "dbupdater_interface.h"
#include <QTimer>
#include <QProcess>

MainWindow::MainWindow()
  : KXmlGuiWindow()
  , m_GLWindow(new GLWindow())
  , m_preferences(new KV::PreferencesDialog(this))
  , m_updater(new UpdaterInterface(this))
{
  setWindowTitle(qAppName());

  setCentralWidget(QWidget::createWindowContainer(m_GLWindow, this));
  connect(m_GLWindow, &GLWindow::pointerChanged, this, [this] (const WGS84Point& p) {
    statusBar()->showMessage(p.print());
  });
  addActions();
  setupGUI();
  QMetaObject::connectSlotsByName(this);
  readSettings();

  auto bus = QDBusConnection::sessionBus();
  actionCollection()->action("checkCharts")->setEnabled(bus.isConnected());


  updateChartSets();
  // save the chart set name for initialization in glwindow
  auto curr = currentChartSet();
  if (!curr.isEmpty()) {
    Conf::MainWindow::setChartset(curr);
    Conf::MainWindow::self()->save();
  }

  auto chartGroup = chartSetGroup();
  connect(chartGroup, &QActionGroup::triggered, this, [this] (QAction* a) {
    qDebug() << "setting chartset" <<  a->objectName();
    m_GLWindow->setChartSet(a->objectName());
  });

  connect(ChartManager::instance(), &ChartManager::chartSetsUpdated,
          this, &MainWindow::updateChartSets);

}

QActionGroup* MainWindow::chartSetGroup() {
  for (auto group: actionCollection()->actionGroups()) {
    if (group->objectName() == "chartSets") {
      return group;
    }
  }

  auto chartgroup = new QActionGroup(this);
  chartgroup->setObjectName("chartSets");
  return chartgroup;
}

void MainWindow::updateChartSets() {

  QList<QAction*>::const_iterator p = std::find_if(menuBar()->actions().cbegin(),
                                                   menuBar()->actions().cend(),
                                                   [] (const QAction* a) {
    return a->objectName() == "charts";
  });
  auto chartmenu = (*p)->menu();

  auto curr = currentChartSet();
  qDebug() << "current chartset" << curr;


  auto chartgroup = chartSetGroup();
  auto prevActions = chartgroup->actions();
  auto chartsets = ChartManager::instance()->chartSets();
  for (auto s: chartsets) {
    if (actionCollection()->action(s) != nullptr) {
      prevActions.removeOne(actionCollection()->action(s));
      qDebug()  << actionCollection()->action(s)->objectName() << "already present";
      continue;
    }
    qDebug()  << "adding" << s;
    auto a = new QAction(this);
    a->setText(s);
    a->setCheckable(true);
    chartgroup->addAction(a);
    chartmenu->addAction(a);
    actionCollection()->addAction(s, a);
  }

  for (auto act: prevActions) {
    qDebug()  << "removing" << act->objectName();
    chartgroup->removeAction(act);
    chartmenu->removeAction(act);
    actionCollection()->removeAction(act);
  }

  if (!chartsets.isEmpty()) {
    QString sel;
    if (!curr.isEmpty() && actionCollection()->action(curr) != nullptr) {
      sel = curr;
    } else {
      sel = Conf::MainWindow::chartset();
    }
    if (!chartsets.contains(sel)) sel = chartsets.first();
    qDebug() << "current chartset" << sel;
    actionCollection()->action(sel)->setChecked(true);
    m_GLWindow->setChartSet(sel, true);
  }
}

void MainWindow::on_quit_triggered() {
  close();
}

void MainWindow::on_northUp_triggered() {
  m_GLWindow->northUp();
}

void MainWindow::on_fullScreen_toggled(bool on) {
  if (on) {
    m_fallbackGeom = geometry();
    showFullScreen();
  } else {
    showNormal();
  }
}

void MainWindow::on_panNorth_triggered() {
  m_GLWindow->compassPan(Angle::fromDegrees(0));
}

void MainWindow::on_panEast_triggered() {
  m_GLWindow->compassPan(Angle::fromDegrees(90));
}

void MainWindow::on_panSouth_triggered() {
  m_GLWindow->compassPan(Angle::fromDegrees(180));
}

void MainWindow::on_panWest_triggered() {
  m_GLWindow->compassPan(Angle::fromDegrees(270));
}

void MainWindow::on_rotateCW_triggered() {
  m_GLWindow->rotateEye(Angle::fromDegrees(-1.));
}

void MainWindow::on_rotateCCW_triggered() {
  m_GLWindow->rotateEye(Angle::fromDegrees(1.));
}

void MainWindow::on_zoomIn_triggered() {
  m_GLWindow->zoomIn();
}

void MainWindow::on_zoomOut_triggered() {
  m_GLWindow->zoomOut();
}

void MainWindow::on_preferences_triggered() {
  m_preferences->show();
}

void MainWindow::on_checkCharts_triggered() {
  auto resp = m_updater->ping();
  // qDebug() << resp.isValid();
  if (!resp.isValid()) {
    qDebug() << resp.error().name() << resp.error().message();
    qDebug() << "Launching dbupdater";
    auto updater = QString("%1_dbupdater").arg(qAppName());
    bool ok = QProcess::startDetached(updater, QStringList());
    qDebug() << "Launched" << updater << ", status =" << ok;
    if (ok) {
      QTimer::singleShot(200, this, &MainWindow::on_checkCharts_triggered);
    }
  } else {
    m_updater->sync();
  }
}

void MainWindow::readSettings() {
  m_fallbackGeom = QRect();
  if (Conf::MainWindow::fullScreen()) {
    // go fullscreen when MainWindow::show() is called
    setWindowState(Qt::WindowFullScreen);
    // block toggle() so that we won't go fullscreen in on_fullScreen_toggled
    const bool blocked = action("fullScreen")->blockSignals(true);
    action("fullScreen")->setChecked(true);
    action("fullScreen")->blockSignals(blocked);
    m_fallbackGeom = Conf::MainWindow::lastGeom();
    if (m_fallbackGeom.isValid()) {
      setGeometry(m_fallbackGeom);
    }
  }
}

QString MainWindow::currentChartSet() const {
  QList<QActionGroup*> groups = actionCollection()->actionGroups();
  for (auto group: groups) {
    if (group->objectName() == "chartSets") {
      auto act = group->checkedAction();
      if (act == nullptr) {
        return QString();
      }
      return act->objectName();
    }
  }
  return QString();
}

void MainWindow::writeSettings() {
  if (m_fallbackGeom.isValid()) {
    Conf::MainWindow::setLastGeom(m_fallbackGeom);
  }
  Conf::MainWindow::setFullScreen(action("fullScreen")->isChecked());

  auto curr = currentChartSet();
  if (!curr.isEmpty()) {
    Conf::MainWindow::setChartset(curr);
  }

  Conf::MainWindow::self()->save();
  Conf::MarinerParams::self()->save();
}

void MainWindow::addActions() {
  struct Data {
    QString name;
    QString text;
    QString shortcut;
    QString theme;
    QString tooltip;
    bool enabled;
    bool checkable;
    bool checked;
  };

  const QVector<Data> actionData{
    {"quit", "Quit", "Ctrl+Q", "application-exit", "Quit", true, false, false},
    {"fullScreen", "Fullscreen", "Ctrl+Shift+F11", "", "View in full screen", true, true, false},
    {"zoomIn", "Zoom In", "Ctrl++", "zoom-in", "", true, false, false},
    {"zoomOut", "Zoom Out", "Ctrl+-", "zoom-out", "", true, false, false},
    {"panNorth", "Pan North", "Up", "", "", true, false, false},
    {"panEast", "Pan East", "Right", "", "", true, false, false},
    {"panSouth", "Pan South", "Down", "", "", true, false, false},
    {"panWest", "Pan West", "Left", "", "", true, false, false},
    {"northUp", "North Up", "N", "", "Reset North upwards", true, false, false},
    {"rotateCCW", "Rotate CCW", "B", "", "Rotate chart counterclockwise", true, false, false},
    {"rotateCW", "Rotate CW", "M", "", "Rotate chart clockwise", true, false, false},
    {"preferences", "Preferences...", "", "configure", "", true, false, false},
    {"checkCharts", "Update charts", "Ctrl+Shift+U", "", "", true, false, false},
  };

  for (auto d: actionData) {
    auto a = new QAction(this);
    actionCollection()->addAction(d.name, a);
    a->setText(d.text);
    a->setCheckable(d.checkable);
    if (d.checkable) {
      a->setChecked(d.checked);
    }
    if (!d.theme.isEmpty()) {
      a->setIcon(QIcon::fromTheme(d.theme));
    }
    a->setToolTip(d.tooltip);
    if (!d.shortcut.isEmpty()) {
      actionCollection()->setDefaultShortcut(a, d.shortcut);
    }
    a->setEnabled(d.enabled);

  }

  // Customized help menu

  auto help = new KHelpMenu(this, KAboutData::applicationData(), true);

  QAction *whatsThisAction = help->action(KHelpMenu::menuWhatsThis);
  actionCollection()->addAction(whatsThisAction->objectName(), whatsThisAction);

  QAction *reportBugAction = help->action(KHelpMenu::menuReportBug);
  actionCollection()->addAction(reportBugAction->objectName(), reportBugAction);

  QAction *aboutAppAction = help->action(KHelpMenu::menuAboutApp);
  actionCollection()->addAction(aboutAppAction->objectName(), aboutAppAction);

  QAction *aboutKdeAction = help->action(KHelpMenu::menuAboutKDE);
  actionCollection()->addAction(aboutKdeAction->objectName(), aboutKdeAction);


  setHelpMenuEnabled(false);

}

void MainWindow::closeEvent(QCloseEvent* event) {
  writeSettings();
  m_GLWindow->saveState();
  event->accept();
}

