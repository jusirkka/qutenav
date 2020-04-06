#include "mainwindow.h"
#include "glwidget.h"
#include "globe.h"
#include <KActionCollection>
#include <KHelpMenu>
#include <KAboutData>
#include <QApplication>
#include "conf_mainwindow.h"
#include <QCloseEvent>

MainWindow::MainWindow()
  : KXmlGuiWindow()
  , m_GLWidget(new GLWidget(this))
{
  setWindowTitle(qAppName());

  setCentralWidget(m_GLWidget);
  addActions();
  setupGUI();
  QMetaObject::connectSlotsByName(this);
  readSettings();
}

void MainWindow::on_quit_triggered() {
  close();
}

void MainWindow::on_northUp_triggered() {
  m_GLWidget->northUp();
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
  // TODO
}

void MainWindow::on_panEast_triggered() {
  // TODO
}

void MainWindow::on_panSouth_triggered() {
  // TODO
}

void MainWindow::on_panWest_triggered() {
  // TODO
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
  Conf::MainWindow::self()->save();
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
    {"northUp", "North Up", "N", "", "Rotate north to upwards direction", true, false, false},
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
  m_GLWidget->saveState();
  event->accept();
}

