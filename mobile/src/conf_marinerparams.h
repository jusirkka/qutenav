#pragma once

#include "configgroup.h"
#include "settings.h"

namespace Conf {


class MarinerParams: public ConfigGroup {
public:

  static MarinerParams *self();
  ~MarinerParams();

  class EnumMaxCategory {
  public:
    enum type {Base, Standard, Other, Mariners};
  };

  CONF_DECL(twoShades, TwoShades, two_shades, bool, toBool)
  CONF_DECL(safetyContour, SafetyContour, safety_contour, double, toDouble)
  CONF_DECL(safetyDepth, SafetyDepth, safety_depth, double, toDouble)
  CONF_DECL(shallowContour, ShallowContour, shallow_contour, double, toDouble)
  CONF_DECL(deepContour, DeepContour, deep_contour, double, toDouble)
  CONF_DECL(shallowPattern, ShallowPattern, shallow_pattern, bool, toBool)
  CONF_DECL(plainBoundaries, PlainBoundaries, plain_boundaries, bool, toBool)
  CONF_DECL(simplifiedSymbols, SimplifiedSymbols, simplified_symbols, bool, toBool)
  CONF_DECL(showMeta, ShowMeta, show_meta, bool, toBool)
  CONF_DECL(fullLengthSectors, FullLengthSectors, full_length_sectors, bool, toBool)

  static void setColorTable(Settings::ColorTable v) {
    self()->m_values["color_table"] = static_cast<uint>(v);
  }
  static Settings::ColorTable colorTable() {
    return static_cast<Settings::ColorTable>(self()->m_values["color_table"].toUInt());
  }

  static void setMaxCategory(EnumMaxCategory::type v) {
    self()->m_values["max_category"] = static_cast<uint>(v);
  }
  static EnumMaxCategory::type maxCategory() {
    return static_cast<EnumMaxCategory::type>(self()->m_values["max_category"].toUInt());
  }

  static void setTextGrouping(const QList<int> & v) {
    QVariantList items;
    for (auto i: v) items.append(i);
    self()->m_values["text_grouping"] = items;
  }

  static QList<int> textGrouping() {
    QList<int> items;
    QVariantList vitems = self()->m_values["text_grouping"].toList();
    for (auto v: vitems) items.append(v.toInt());
    return items;
  }

private:

  MarinerParams();
};

}

