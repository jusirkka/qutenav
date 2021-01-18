#include "conf_detailmode.h"

Conf::DetailMode* Conf::DetailMode::self() {
  static DetailMode* s = new DetailMode();
  return s;
}

Conf::DetailMode::DetailMode()
  : ConfigGroup("DetailMode", "qopencpnrc")
{

  m_defaults["width_mm"] = 240.;
  m_defaults["height_mm"] = 135.;
  m_defaults["name"] = "OutlineMode";
  m_defaults["projection"] = "SimpleMercator";
  m_defaults["scale"] = 30000000;
  m_defaults["eye"] = "+30.93+134.842CRSWGS_84/";
  m_defaults["north_angle"] = 0.;

  load();
}

Conf::DetailMode::~DetailMode() {}

