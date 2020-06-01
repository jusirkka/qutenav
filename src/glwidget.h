#pragma once

#include <QOpenGLWidget>
#include <QOpenGLVertexArrayObject>

#include "detailmode.h"

class QOpenGLDebugLogger;
class ChartManager;

class GLWidget: public QOpenGLWidget
{

  Q_OBJECT

public:

  GLWidget(QWidget* parent = nullptr);
  ~GLWidget();
  void saveState();
  void northUp();
  void zoomIn();
  void zoomOut();
  void compassPan(Angle bearing, float pixels = 1.);

protected:

  void initializeGL() override;
  void resizeGL(int w, int h) override;
  void paintGL() override;

  void mousePressEvent(QMouseEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void mouseDoubleClickEvent(QMouseEvent *event) override;
  void wheelEvent(QWheelEvent *event) override;

signals:

  void updateViewport(const Camera* cam);

private slots:

  void pan();

  void initializeChartMode();
  void finalizeChartMode();

  void updateCharts();
  void updateObjects();

private:


  QOpenGLVertexArrayObject m_vao;
  DetailMode* m_mode;
  QOpenGLDebugLogger* m_logger;
  ChartManager* m_manager;

  QPoint m_diff;
  QPoint m_lastPos;
  bool m_gravity;
  QTimer* m_timer;
  quint64 m_moveCounter;

};

