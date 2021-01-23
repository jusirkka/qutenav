#pragma once

#include "drawable.h"
#include <QOpenGLBuffer>
#include <glm/glm.hpp>
#include <QColor>

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

  using DataVector = QVector<glm::vec4>;

  struct Rectangle {
    Rectangle(const GL::VertexVector& d, int i, size_t off);
    Rectangle() = default;
    DataVector outline;
    QVector3D center;
    QColor color;
    int offset;
  };

  QVector<Rectangle> m_outlines;

  struct _locations {
    int base_color;
    int m_pv;
    int center;
    int angle;
    int vertexOffset;
  } m_locations;

  ChartManager* m_manager;

};

