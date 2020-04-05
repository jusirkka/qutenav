#pragma once

#include <QOpenGLWidget>
#include <QOpenGLVertexArrayObject>

#include "detailmode.h"

class QOpenGLDebugLogger;

class GLWidget: public QOpenGLWidget
{

  Q_OBJECT

public:

  GLWidget(QWidget* parent = nullptr);
  ~GLWidget();
  void saveState();
  void northUp();

protected:

  void initializeGL() override;
  void resizeGL(int w, int h) override;
  void paintGL() override;

  void mousePressEvent(QMouseEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void mouseDoubleClickEvent(QMouseEvent *event) override;
  void wheelEvent(QWheelEvent *event) override;

private slots:

  void zoomIn();
  void zoomOut();
  void pan();

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

