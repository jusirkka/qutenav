#pragma once

#include "drawable.h"
#include <QOpenGLBuffer>
#include <QVector4D>

class ChartManager;

class Outliner: public Drawable {

  Q_OBJECT

public:

  Outliner(QObject* parent);

  void paintGL(const Camera* cam) override;
  void initializeGL() override;
  void updateCharts(const Camera* cam, const QRectF& viewArea) override;

  ~Outliner() = default;

private:

  void updateBuffers();

  QOpenGLBuffer m_coordBuffer;
  QOpenGLShaderProgram* m_program;

  using DataVector = QVector<GLfloat>;

  struct Rectangle {
    Rectangle(const DataVector& d, int i);
    DataVector outline;
    QVector3D center;
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

  ChartManager* m_manager;

};

