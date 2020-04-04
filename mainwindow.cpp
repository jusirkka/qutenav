#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "glwidget.h"
#include "globe.h"
#include <QSettings>

MainWindow::MainWindow(QWidget *parent)
  : QMainWindow(parent)
  , m_UI(new Ui::MainWindow)
  , m_GLWidget(new GLWidget(this))
{
   m_UI->setupUi(this);
   setCentralWidget(m_GLWidget);
   readSettings();
}

void MainWindow::on_actionQuit_triggered() {
  close();
}

void MainWindow::on_actionNorthUp_triggered() {
  m_GLWidget->northUp();
}

MainWindow::~MainWindow() {
}

void MainWindow::readSettings() {
  QSettings settings;
  restoreGeometry(settings.value("geometry").toByteArray());
  restoreState(settings.value("windowState").toByteArray());
}

void MainWindow::closeEvent(QCloseEvent *event)
{
  QSettings settings;
  settings.setValue("geometry", saveGeometry());
  settings.setValue("windowState", saveState());
  m_GLWidget->saveState();
  QMainWindow::closeEvent(event);
}

