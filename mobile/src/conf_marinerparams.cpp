/* -*- coding: utf-8-unix -*-
 *
 * File: mobile/src/conf_marinerparams.cpp
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
  m_defaults["deep_contour"] = 10.;
  m_defaults["shallow_pattern"] = true;
  m_defaults["color_table"] = static_cast<uint>(EnumColorTable::type::DayBright);
  m_defaults["plain_boundaries"] = false;
  m_defaults["simplified_symbols"] = false;
  m_defaults["max_category"] = static_cast<uint>(EnumMaxCategory::type::Mariners);
  m_defaults["show_meta"] = true;
  m_defaults["full_length_sectors"] = false;

  QVariantList items;
  items << 10 << 11 << 12 << 20 << 21 << 23 << 24 << 25 << 26 << 27 << 28 << 29 << 31;
  m_defaults["text_grouping"] = items;

  load();

  QVariantList vitems = m_values["text_grouping"].toList();
  for (auto v: vitems) m_textGrouping.append(v.toInt());
}

Conf::MarinerParams::~MarinerParams() {}
