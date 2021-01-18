#include "conf_marinerparams.h"

Conf::MarinerParams* Conf::MarinerParams::self() {
  static MarinerParams* s = new MarinerParams();
  return s;
}

Conf::MarinerParams::MarinerParams()
  : ConfigGroup("MarinerParams", "qopencpnrc")
{

  m_defaults["two_shades"] = false;
  m_defaults["safety_contour"] = 3.;
  m_defaults["safety_depth"] = 3.;
  m_defaults["shallow_contour"] = 2.;
  m_defaults["deep_contour"] = 6.;
  m_defaults["shallow_pattern"] = true;
  m_defaults["color_table"] = static_cast<uint>(Settings::DayBright);
  m_defaults["plain_boundaries"] = false;
  m_defaults["simplified_symbols"] = false;
  m_defaults["max_category"] = static_cast<uint>(EnumMaxCategory::type::Mariners);
  m_defaults["show_meta"] = true;
  m_defaults["full_length_sectors"] = false;

  QVariantList items;
  items << 10 << 11 << 20 << 21 << 22 << 23 << 24 << 25 << 26 << 27 << 29 << 31;
  m_defaults["text_grouping"] = items;

  load();
}

Conf::MarinerParams::~MarinerParams() {}
