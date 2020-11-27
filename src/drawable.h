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

  virtual ~Drawable() = default;

};

using DrawableVector = QVector<Drawable*>;
