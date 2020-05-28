#pragma once

#include "drawable.h"
#include <QOpenGLBuffer>
#include <s57object.h>

class S57Chart;

class GeoProjection;

class ChartPainter: public Drawable {
  Q_OBJECT

public:
  ChartPainter(GeoProjection* p, QObject* parent);

  void paintGL(const Camera* cam) override;
  void initializeGL() override;
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
  using ChartVector = QVector<S57Chart*>;

  ChartVector m_charts;
  ChartDataVector m_chartData;
};

