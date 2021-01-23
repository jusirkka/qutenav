#pragma once

#include "drawable.h"
#include <QOpenGLBuffer>
#include <QColor>


class Globe: public Drawable
{

  Q_OBJECT

public:

  Globe(QObject* parent);

  void paintGL(const Camera* cam) override;
  void initializeGL() override;
  void updateCharts(const Camera* cam, const QRectF& viewArea) override;

  ~Globe() = default;

private:

  void initLayers();
  int indexFromChartName(const QString& name);

  QOpenGLBuffer m_coordBuffer;
  QOpenGLBuffer m_indexBuffer;
  QOpenGLShaderProgram* m_program;

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

  struct ElementData {
    uintptr_t offset;
    size_t size;
  };

  struct _layer_data {
    QColor color;
    ElementData elem;
    size_t offset;
  } m_layerData[LAYERS];


  Mesh m_coords;
  Indices m_strips;
};

