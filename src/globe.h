#pragma once

#include "drawable.h"
#include <QOpenGLBuffer>
#include <QColor>

class QDir;
class QOpenGLTexture;

class Globe: public Drawable
{

  Q_OBJECT

public:

  Globe(QObject* parent);

  void paintGL(const Camera* cam) override;
  void initializeGL() override;
  void updateCharts(const Camera* cam, const QRectF& viewArea) override;

  ~Globe();

private:

  void loadImages(const QDir& imageDir);
  void loadImage(const QDir& imageDir);

  QOpenGLBuffer m_coordBuffer;
  QOpenGLBuffer m_indexBuffer;
  QOpenGLShaderProgram* m_program;
  QOpenGLTexture* m_globeTexture;

  GLsizei m_indexCount;

  struct _locations {
    int globe;
    int m_pv;
    int lp;
  } m_locations;

};

