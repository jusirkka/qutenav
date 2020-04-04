#include "chartmode.h"
#include "orthocam.h"
#include "outlinemode.h"

ChartMode::ChartMode(float wmm, float hmm) {
  m_camera = new OrthoCam(wmm, hmm);
}


DetailMode* ChartMode::smallerScaleMode(float /*wmm*/, float /*hmm*/) const {
  return nullptr;
}

DetailMode* ChartMode::largerScaleMode(float wmm, float hmm) const {
  DetailMode* outlines = new OutlineMode(wmm, hmm);
  const quint32 scale = qMax(m_camera->scale(), outlines->camera()->minScale());
  outlines->camera()->setScale(scale);
  outlines->camera()->reset(m_camera->eye(), m_camera->northAngle());
  return outlines;
}
