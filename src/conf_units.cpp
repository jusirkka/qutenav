/* -*- coding: utf-8-unix -*-
 *
 * File: mobile/src/conf_units.cpp
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
#include "conf_units.h"

Conf::Units* Conf::Units::self() {
  static Units* s = new Units();
  return s;
}

Conf::Units::Units()
  : ConfigGroup("Units", "qutenavrc")
{

  m_defaults["location"] = static_cast<uint>(EnumLocation::type::DegMin);
  m_defaults["depth"] = static_cast<uint>(EnumDepth::type::Meters);
  m_defaults["distance"] = static_cast<uint>(EnumDistance::type::KM);
  m_defaults["short_distance"] = static_cast<uint>(EnumShortDistance::type::M);
  m_defaults["boat_speed"] = static_cast<uint>(EnumBoatSpeed::type::Kn);


  load();

}

Conf::Units::~Units() {}
