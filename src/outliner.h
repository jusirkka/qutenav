#pragma once

#include "drawable.h"
#include <QOpenGLBuffer>
#include <QVector4D>

class Outliner: public Drawable
{
public:

  Outliner();

  void paintGL(const Camera* cam) override;
  void initializeGL(QOpenGLWidget* context) override;

private:

  QOpenGLBuffer m_coordBuffer;

  using DataVector = QVector<GLfloat>;

  struct Rectangle {
    Rectangle(const DataVector& d, size_t offset);
    QVector3D center;
    DataVector outline;
    QColor color;
    size_t offset;
  };

  QVector<Rectangle> m_outlines;

  struct _locations {
    int base_color;
    int m_pv;
    int center;
    int angle;
  } m_locations;


};

