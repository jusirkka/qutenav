#include "detailmode.h"
#include "chartmode.h"
#include "outlinemode.h"
#include <QSettings>

DetailMode* DetailMode::RestoreState() {
  QSettings settings;
  QString name = settings.value("mode", "OutlineMode").toString();

  bool ok;
  float wmm = settings.value("width-mm", 160.).toFloat(&ok);
  if (!ok) wmm = 160.;
  float hmm = settings.value("height-mm", 90.).toFloat(&ok);
  if (!ok) hmm = 90.;

  DetailMode* mode;
  if (name == "OutlineMode") {
    mode = new OutlineMode(wmm, hmm);
  } else if (name == "ChartMode") {
    mode = new ChartMode(wmm, hmm);
  } else {
    throw ModeError("unsupported mode");
  }
  quint32 scale = settings.value("scale", mode->camera()->scale()).toULongLong(&ok);
  if (!ok) scale = mode->camera()->scale();
  scale = qMin(scale, mode->camera()->maxScale());
  scale = qMax(scale, mode->camera()->minScale());
  mode->camera()->setScale(scale);

  WGS84Point def = mode->camera()->eye();
  WGS84Point e = WGS84Point::parseISO6709(settings.value("eye", def.toISO6709()).toString());
  if (!e.valid()) e = def;

  float a = settings.value("north-angle", mode->camera()->northAngle().degrees()).toFloat(&ok);
  if (!ok) a = mode->camera()->northAngle().degrees();

  mode->camera()->reset(e, Angle::fromDegrees(a));

  return mode;
}

void DetailMode::saveState(float wmm, float hmm) const {
  QSettings settings;
  settings.setValue("mode", className());

  settings.setValue("width-mm", wmm);
  settings.setValue("height-mm", hmm);

  settings.setValue("scale", m_camera->scale());


  settings.setValue("eye", m_camera->eye().toISO6709());

  settings.setValue("north-angle", m_camera->northAngle().degrees());
}
