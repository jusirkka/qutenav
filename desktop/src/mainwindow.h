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

