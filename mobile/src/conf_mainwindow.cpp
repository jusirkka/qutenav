#include "conf_mainwindow.h"

Conf::MainWindow* Conf::MainWindow::self() {
  static MainWindow* s = new MainWindow();
  return s;
}

Conf::MainWindow::MainWindow()
  : ConfigGroup("MainWindow", "qopencpnrc")
{
  m_defaults["chartset"] = "None";

  load();
}

Conf::MainWindow::~MainWindow() {/*noop*/}
