/* -*- coding: utf-8-unix -*-
 *
 * File: mobile/src/conf_marinerparams.h
 *
 * Copyright (C) 2021 Jukka Sirkka
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once

#include "configgroup.h"

namespace Conf {


class MarinerParams: public ConfigGroup {
public:

  static MarinerParams *self();
  ~MarinerParams();

  class EnumMaxCategory {
  public:
    enum type {Base, Standard, Other, Mariners};
    static const inline QStringList names {
      //% "Base"
      QT_TRID_NOOP("qtnav-category-base"),
      //% "Standard"
      QT_TRID_NOOP("qtnav-category-standard"),
      //% "Other"
      QT_TRID_NOOP("qtnav-category-other"),
      //% "Mariners"
      QT_TRID_NOOP("qtnav-category-mariners"),
    };

  };

  class EnumColorTable {
  public:
    enum type {DayBright, DayBlackBack, DayWhiteBack, Dusk, Night};
    static const inline QStringList names {
      //% "Day Bright"
      QT_TRID_NOOP("qtnav-colortable-day-bright"),
      //% "Day Black/Bg"
      QT_TRID_NOOP("qtnav-colortable-day-black-bg"),
      //% "Day White/Bg"
      QT_TRID_NOOP("qtnav-colortable-day-white-bg"),
      //% "Dusk"
      QT_TRID_NOOP("qtnav-colortable-dusk"),
      //% "Night"
      QT_TRID_NOOP("qtnav-colortable-night"),
    };

  };

  CONF_DECL(TwoShades, two_shades, bool, toBool)
  CONF_DECL(SafetyContour, safety_contour, double, toDouble)
  CONF_DECL(SafetyDepth, safety_depth, double, toDouble)
  CONF_DECL(ShallowContour, shallow_contour, double, toDouble)
  CONF_DECL(DeepContour, deep_contour, double, toDouble)
  CONF_DECL(ShallowPattern, shallow_pattern, bool, toBool)
  CONF_DECL(PlainBoundaries, plain_boundaries, bool, toBool)
  CONF_DECL(SimplifiedSymbols, simplified_symbols, bool, toBool)
  CONF_DECL(FullLengthSectors, full_length_sectors, bool, toBool)

  static void setColorTable(EnumColorTable::type v) {
    self()->m_values["color_table"] = static_cast<uint>(v);
  }
  static EnumColorTable::type ColorTable() {
    return static_cast<EnumColorTable::type>(self()->m_values["color_table"].toUInt());
  }

  static void setMaxCategory(EnumMaxCategory::type v) {
    self()->m_values["max_category"] = static_cast<uint>(v);
  }
  static EnumMaxCategory::type MaxCategory() {
    return static_cast<EnumMaxCategory::type>(self()->m_values["max_category"].toUInt());
  }

  static void setTextGrouping(const QList<int>& v) {
    self()->m_textGrouping = v;

    QVariantList items;
    for (auto i: v) items.append(i);

    self()->m_values["text_grouping"] = items;
  }

  static QList<int> TextGrouping() {
    return self()->m_textGrouping;
  }

  static void setDisabledClasses(const QList<int>& v) {
    self()->m_disabledClasses = v;

    QVariantList items;
    for (auto i: v) items.append(i);

    self()->m_values["disabled_classes"] = items;
  }

  static QList<int> DisabledClasses() {
    return self()->m_disabledClasses;
  }

private:

  MarinerParams();

  QList<int> m_textGrouping;
  QList<int> m_disabledClasses;

};

}

