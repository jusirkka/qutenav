/* -*- coding: utf-8-unix -*-
 *
 * File: mobile/src/conf_detailmode.cpp
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
#include "conf_detailmode.h"

Conf::DetailMode* Conf::DetailMode::self() {
  static DetailMode* s = new DetailMode();
  return s;
}

Conf::DetailMode::DetailMode()
  : ConfigGroup("DetailMode", "qutenavrc")
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

