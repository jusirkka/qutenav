#pragma once

#include "drawable.h"
#include <QOpenGLBuffer>
#include <s57object.h>

class S57Chart;
class GeoProjection;
class ChartManager;

namespace GL {
class AreaShader;
class SolidLineShader;
class DashedLineShader;
}

class ChartPainter: public Drawable {
  Q_OBJECT

public:
  ChartPainter(QObject* parent);

  void paintGL(const Camera* cam) override;
  void initializeGL() override;

  ~ChartPainter() = default;

private:

  ChartManager* m_manager;

  GL::AreaShader* m_areaShader;
  GL::SolidLineShader* m_solidShader;
  GL::DashedLineShader* m_dashedShader;

};

