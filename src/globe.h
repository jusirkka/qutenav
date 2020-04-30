#pragma once

#include "drawable.h"
#include <QOpenGLBuffer>


class Globe: public Drawable
{
public:

  Globe();

  void paintGL(const Camera* cam) override;
  void initializeGL(QOpenGLWidget* context) override;

private:

  void initLayers();
  int indexFromChartName(const QString& name);

  QOpenGLBuffer m_coordBuffer;
  QOpenGLBuffer m_indexBuffer;

  struct _locations {
    int base_color;
    int m_pv;
    int lp;
  } m_locations;

  static const int LAYERS = 7;

  using Mesh = QVector<GLfloat>;
  using Indices = QVector<GLuint>;
  using Offsets = QVector<uintptr_t>;
  using Sizes = QVector<GLsizei>;

  struct StripData {
    uintptr_t offset;
    size_t size;
  };

  using Strips = QVector<StripData>;

  struct _layer_data {
    QColor color;
    Strips strips;
    size_t offset;
  } m_layerData[LAYERS];


  Mesh m_coords;
  Indices m_strips;
};

