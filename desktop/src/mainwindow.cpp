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

MainWindow::MainWindow()
  : KXmlGuiWindow()
  , m_GLWindow(new GLWindow())
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

  QList<QAction*>::const_iterator p = std::find_if(menuBar()->actions().cbegin(),
                                                   menuBar()->actions().cend(),
                                                   [] (const QAction* a) {
    return a->objectName() == "charts";
  });
  auto chartmenu = (*p)->menu();

  auto chartgroup = new QActionGroup(this);
  auto chartsets = ChartManager::instance()->chartSets();
  for (auto s: chartsets) {
    auto a = new QAction(this);
    a->setText(s);
    a->setCheckable(true);
    chartgroup->addAction(a);
    chartmenu->addAction(a);
    actionCollection()->addAction(s, a);
  }

  connect(chartmenu, &QMenu::triggered, this, [this] (QAction* a) {
    m_GLWindow->setChartSet(a->objectName());
  });

  if (!chartsets.isEmpty()) {
    auto sel = Conf::MainWindow::chartset();
    if (!chartsets.contains(sel)) sel = chartsets.first();
    actionCollection()->action(sel)->setChecked(true);
    // save the chart set name for initialization in glwindow
    Conf::MainWindow::setChartset(sel);
    Conf::MainWindow::self()->save();
  }

  m_preferences = new KV::PreferencesDialog(this);
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

void MainWindow::writeSettings() {
  if (m_fallbackGeom.isValid()) {
    Conf::MainWindow::setLastGeom(m_fallbackGeom);
  }
  Conf::MainWindow::setFullScreen(action("fullScreen")->isChecked());
  QList<QActionGroup*> groups = actionCollection()->actionGroups();
  if (!groups.isEmpty()) {
    Conf::MainWindow::setChartset(groups.first()->checkedAction()->objectName());
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

