#pragma once

#include "drawable.h"
#include <QOpenGLBuffer>
#include <s57object.h>

class S57Chart;
class GeoProjection;
class ChartManager;
class QOpenGLFramebufferObject;

namespace GL {
class AreaShader;
class SolidLineShader;
class DashedLineShader;
class TextShader;
class RasterSymbolShader;
class TextureShader;
}

class ChartPainter: public Drawable {
  Q_OBJECT

public:
  ChartPainter(QObject* parent);

  void paintGL(const Camera* cam) override;
  void initializeGL() override;
  void updateCharts(const Camera* cam, const QRectF& viewArea) override;

  ~ChartPainter();

private:

  Camera* createBufferCamera(const Camera* cam, const QSizeF& vp) const;

  ChartManager* m_manager;
  GL::AreaShader* m_areaShader;
  GL::SolidLineShader* m_solidShader;
  GL::DashedLineShader* m_dashedShader;
  GL::TextShader* m_textShader;
  GL::RasterSymbolShader* m_rasterShader;
  GL::TextureShader* m_textureShader;

  bool m_initialized;
  QRectF m_viewArea;
  WGS84Point m_ref;
  QSizeF m_bufSize;
  QOpenGLFramebufferObject* m_fbo;

  QOpenGLBuffer m_coordBuffer;
  QOpenGLBuffer m_indexBuffer;

  const float m_dots_per_mm_y;
  const float m_dots_per_mm_x;


};

