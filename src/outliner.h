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

  using OutlineVector = QVector<GLfloat>;

  OutlineVector m_outlines;

  struct _locations {
    int point;
    int base_color;
    int m_pv;
    int eye;
  } m_locations;

  struct _params {
    QColor color;
    GLsizei length;
  } m_Params;


};

