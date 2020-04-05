#pragma once

#include "drawable.h"
#include <QOpenGLBuffer>
#include <QVector4D>

class OGRGeometry;
class OGRSpatialReference;

class Globe: public Drawable
{
public:

  Globe();

  void paintGL(const Camera* cam) override;
  void initializeGL(QOpenGLWidget* context) override;

private:

  void initLayers();
  int indexFromChartName(const QString& name);
  void triangulate(const OGRGeometry* geom);
  QString rectToText(const QRectF& clip);
  OGRGeometry* createGeometry(const QString& txt) const;
  void triangulateSphere();
  using Mesh = QVector<QVector2D>;
  using IndexVector = QVector<quint32>;

  Mesh m_coords;
  IndexVector m_triangles;

  QOpenGLBuffer m_coordBuffer;
  QOpenGLBuffer m_indexBuffer;
  GLsizei m_indexCount;

  struct _locations {
    int point;
    int base_color;
    int m_pv;
    int lp;
    int eye;
    int layer_index;
  } m_locations;

  static const int LAYERS = 7;

  struct _layer_data {
    QColor color;
    GLuint offset;
    GLsizei length;
  } m_layerData[LAYERS];



};

