#pragma once


#include <QMatrix4x4>
#include <QOpenGLShaderProgram>
#include <QOpenGLWidget>


#include "camera.h"

class Drawable
{
public:

  Drawable() = default;
  virtual void paintGL(const Camera* camera) = 0;
  // OpenGL related stuff to be initialized after we have a GL context.
  virtual void initializeGL(QOpenGLWidget* context) = 0;
  virtual ~Drawable() = default;

protected:

    QOpenGLShaderProgram* m_program;

};

using DrawableVector = QVector<Drawable*>;

class ChartFileError {
public:
  ChartFileError(QString msg): m_detail(std::move(msg)) {}
  const QString msg() const {return m_detail;}
private:
  QString m_detail;
};
