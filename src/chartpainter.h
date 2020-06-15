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

  struct _tri_locations {
    int base_color;
    int m_pvm;
    int depth;
  } m_tri_locations;

  struct _line_locations {
    int base_color;
    int m_pvm;
    int depth;
    int screenWidth;
    int screenHeight;
    int lineWidth;
    int pattern;
    int patlen;
    int factor;
  } m_line_locations;


  static constexpr uint linePatlen = 18;
  static constexpr uint linefactor = 1;

  struct ChartData {
    GLsizei vertexOffset;
    uintptr_t elementOffset;
    S57::PaintDataVector lineData;
    S57::PaintDataVector triangleData;
  };

  using ChartDataVector = QVector<QVector<ChartData>>;
  ChartDataVector m_chartData;

  WGS84PointVector m_transforms;

  ChartManager* m_manager;

  QOpenGLShaderProgram* m_lineProg;
  QOpenGLShaderProgram* current;
};

