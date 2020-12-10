#pragma once

#include <KXmlGuiWindow>
#include <QObject>


class GLWindow;

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
  void on_zoomIn_triggered();
  void on_zoomOut_triggered();

private:

  void readSettings();
  void writeSettings();
  void addActions();

private:

  GLWindow* m_GLWindow;
  QRect m_fallbackGeom;

};

