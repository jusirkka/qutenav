#pragma once

#include "drawable.h"
#include <QOpenGLBuffer>
#include <s57object.h>

class S57Chart;
class GeoProjection;
class ChartManager;

class ChartPainter: public Drawable {
  Q_OBJECT

public:
  ChartPainter(QObject* parent);

  void paintGL(const Camera* cam) override;
  void initializeGL() override;
  void updateBuffers() override;
  void updateObjects() override;

  ~ChartPainter() = default;

private:

  QOpenGLBuffer m_coordBuffer;
  QOpenGLBuffer m_indexBuffer;

  struct _locations {
    int base_color;
    int m_pvm;
  } m_locations;

  struct ChartData {
    GLsizei vertexOffset;
    uintptr_t elementOffset;
    S57::PaintDataVector lineData;
    S57::PaintDataVector triangleData;
  };

  using ChartDataVector = QVector<QVector<ChartData>>;
  ChartDataVector m_chartData;

  ChartManager* m_manager;
};

