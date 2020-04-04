#include "outlinemode.h"
#include "perscam.h"
#include "globe.h"
#include "outliner.h"
#include "chartmode.h"

OutlineMode::OutlineMode(float wmm, float hmm) {
  m_camera = new PersCam(wmm, hmm);
  m_drawables << new Outliner << new Globe;
}

DetailMode* OutlineMode::largerScaleMode(float /*wmm*/, float /*hmm*/) const {
  return nullptr;
}

DetailMode* OutlineMode::smallerScaleMode(float wmm, float hmm) const {
  DetailMode* charts = new ChartMode(wmm, hmm);
  const quint32 scale = qMin(m_camera->scale(), charts->camera()->maxScale());
  charts->camera()->setScale(scale);
  charts->camera()->reset(m_camera->eye(), m_camera->northAngle());
  return charts;
}
