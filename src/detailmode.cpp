#include "detailmode.h"
#include "chartmode.h"
#include "outlinemode.h"
#include "conf_detailmode.h"

DetailMode::DetailMode(QObject *parent): QObject(parent) {}

DetailMode* DetailMode::RestoreState() {

  const float wmm = Conf::DetailMode::widthMM();
  const float hmm = Conf::DetailMode::heightMM();

  auto p = GeoProjection::CreateProjection(Conf::DetailMode::projection());
  if (p == nullptr) {
    p = new SimpleMercator;
  }

  const QString name = Conf::DetailMode::name();
  DetailMode* mode;
  if (name == "OutlineMode") {
    mode = new OutlineMode(wmm, hmm, p);
  } else if (name == "ChartMode") {
    mode = new ChartMode(wmm, hmm, p);
  } else {
    throw ModeError("unsupported mode");
  }

  quint32 scale = Conf::DetailMode::scale();
  scale = qMin(scale, mode->camera()->maxScale());
  scale = qMax(scale, mode->camera()->minScale());
  mode->camera()->setScale(scale);

  WGS84Point e = WGS84Point::parseISO6709(Conf::DetailMode::eye());
  if (!e.valid()) e = mode->camera()->eye();

  const Angle a = Angle::fromDegrees(Conf::DetailMode::northAngle());

  mode->camera()->reset(e, a);

  return mode;
}

void DetailMode::saveState() const {
  Conf::DetailMode::setName(className());

  const float hmm = m_camera->heightMM();
  const float wmm = hmm * m_camera->aspect();
  Conf::DetailMode::setHeightMM(hmm);
  Conf::DetailMode::setWidthMM(wmm);

  Conf::DetailMode::setScale(m_camera->scale());

  Conf::DetailMode::setEye(m_camera->eye().toISO6709());

  Conf::DetailMode::setNorthAngle(m_camera->northAngle().degrees());

  Conf::DetailMode::setProjection(m_camera->geoprojection()->className());

  Conf::DetailMode::self()->save();

}
