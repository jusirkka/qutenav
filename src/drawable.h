#pragma once


#include <QMatrix4x4>
#include <QOpenGLShaderProgram>
#include <QOpenGLWidget>


#include "camera.h"

class Drawable: public QObject {

  Q_OBJECT

public:

  Drawable(QObject* parent);

  virtual void paintGL(const Camera* camera) = 0;
  // OpenGL related stuff to be initialized after we have a GL context.
  virtual void initializeGL() = 0;
  // called when the set of active charts changes.
  void updateCharts();
  // updates vertex and element buffers if present
  virtual void updateBuffers() = 0;
  // called when the set of visible objects in the set of active charts changes
  virtual void updateObjects() = 0;

  virtual ~Drawable() = default;

protected:

    QOpenGLShaderProgram* m_program;

};

using DrawableVector = QVector<Drawable*>;
