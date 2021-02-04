#pragma once

#include <QtQuick/QQuickFramebufferObject>
#include <QOpenGLVertexArrayObject>

class QQuickWindow;
class DetailMode;

class ChartRenderer: public QQuickFramebufferObject::Renderer {

public:

  ChartRenderer(QQuickWindow *window);

  ~ChartRenderer();

protected:

  void render() override;
  void synchronize(QQuickFramebufferObject* parent) override;
  QOpenGLFramebufferObject* createFramebufferObject(const QSize &size) override;


private:

  bool initializeChartMode();
  bool finalizeChartMode();
  void initializeGL();

  DetailMode* m_mode;
  QQuickWindow* m_window;
  QOpenGLVertexArrayObject m_vao;


};


