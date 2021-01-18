#pragma once

#include <QObject>
#include <QOpenGLFramebufferObject>

class QQuickWindow;
class Camera;
class DetailMode;
class QOpenGLDebugLogger;
class QOpenGLFramebufferObject;

class ChartRenderer: public QObject {

  Q_OBJECT

public:

  ChartRenderer(QQuickWindow *window);
  void setCamera(const Camera* cam);
  Camera* cloneCamera() const;

  void updateCharts(const QRectF& va);
  bool initializeChartMode();
  bool finalizeChartMode();
  void chartSetChanged();
  GLuint textureId() const;

  ~ChartRenderer();

public slots:

  void paint();
  void initializeGL();

private:

  DetailMode* m_mode;
  QQuickWindow* m_window;
  QOpenGLDebugLogger* m_logger;
  QOpenGLFramebufferObject* m_fbo;

};


