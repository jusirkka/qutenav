#include "outlinemode.h"
#include "perscam.h"
#include "globe.h"
#include "outliner.h"
#include "chartmode.h"

OutlineMode::OutlineMode(float wmm, float hmm, GeoProjection* p) {
  m_camera = new PersCam(wmm, hmm, p);
  m_drawables << new Outliner << new Globe;
}

DetailMode* OutlineMode::largerScaleMode() const {
  return nullptr;
}

DetailMode* OutlineMode::smallerScaleMode() const {
  const float hmm = m_camera->heightMM();
  const float wmm = hmm * m_camera->aspect();
  auto p = GeoProjection::CreateProjection(m_camera->geoprojection()->className());
  DetailMode* charts = new ChartMode(wmm, hmm, p);
  const quint32 scale = qMin(m_camera->scale(), charts->camera()->maxScale());
  charts->camera()->setScale(scale);
  charts->camera()->reset(m_camera->eye(), m_camera->northAngle());
  return charts;
}
