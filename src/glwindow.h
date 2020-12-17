#pragma once

#include <QOpenGLWindow>
#include <QOpenGLVertexArrayObject>

#include "detailmode.h"

class QOpenGLDebugLogger;
class ChartManager;
class TextManager;
class RasterSymbolManager;

class GLWindow: public QOpenGLWindow
{

  Q_OBJECT

public:

  GLWindow();
  ~GLWindow();
  void saveState();
  void northUp();
  void zoomIn();
  void zoomOut();
  void compassPan(Angle bearing, float pixels = 1.);
  void rotateEye(Angle amount);

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

  void updateViewport(const Camera* cam, bool force = false);

private slots:

  void pan();

  void initializeChartMode();
  void finalizeChartMode();

  void updateCharts(const QRectF& viewArea);

private:


  QOpenGLVertexArrayObject m_vao;
  DetailMode* m_mode;
  QOpenGLDebugLogger* m_logger;

  QPoint m_diff;
  QPoint m_lastPos;
  bool m_gravity;
  QTimer* m_timer;
  quint64 m_moveCounter;

};

