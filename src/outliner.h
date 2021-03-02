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

  QOpenGLBuffer m_cornerBuffer;
  int m_instances;
  QOpenGLShaderProgram* m_program;

  struct _locations {
    int base_color;
    int m_pv;
    int center;
    int angle;
    int nump;
  } m_locations;


  ChartManager* m_manager;

};

