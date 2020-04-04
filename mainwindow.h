#pragma once

#include <QMainWindow>
#include <QObject>
#include "drawable.h"

namespace Ui {class MainWindow;}

class GLWidget;

class MainWindow: public QMainWindow
{
  Q_OBJECT

public:

  explicit MainWindow(QWidget* parent = nullptr);
  ~MainWindow() override;

protected:

  void closeEvent(QCloseEvent *event) override;

private slots:

  void on_actionQuit_triggered();
  void on_actionNorthUp_triggered();

private:

  void readSettings();

private:

  Ui::MainWindow* m_UI;
  GLWidget* m_GLWidget;

};

