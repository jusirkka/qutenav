/* -*- coding: utf-8-unix -*-
 *
 * File: mobile/src/conf_units.h
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


class Units: public ConfigGroup {
public:

  static Units *self();
  ~Units();

  class EnumLocation {
  public:
    enum type {Deg, DegMin, DegMinSec};
    static const inline QStringList names {
      //% "Degrees"
      QT_TRID_NOOP("qtnav-loc-units-deg"),
      //% "Degrees/Minutes"
      QT_TRID_NOOP("qtnav-loc-units-degmin"),
      //% "Degrees/Minutes/Seconds"
      QT_TRID_NOOP("qtnav-loc-units-degminsec"),
    };
  };

  static void setLocation(EnumLocation::type v) {
    self()->m_values["location"] = static_cast<uint>(v);
  }
  static EnumLocation::type Location() {
    return static_cast<EnumLocation::type>(self()->m_values["location"].toUInt());
  }

  class EnumDepth {
  public:
    enum type {Meters, Fathoms, Feet};
    static const inline QStringList names {
      //% "Meters"
      QT_TRID_NOOP("qtnav-depth-units-meters"),
      //% "Fathoms"
      QT_TRID_NOOP("qtnav-depth-units-fathoms"),
      //% "Feet"
      QT_TRID_NOOP("qtnav-depth-units-feet"),
    };
  };

  static void setDepth(EnumDepth::type v) {
    self()->m_values["depth"] = static_cast<uint>(v);
  }
  static EnumDepth::type Depth() {
    return static_cast<EnumDepth::type>(self()->m_values["depth"].toUInt());
  }

  class EnumDistance {
  public:
    enum type {KM, NM, Mi};
    static const inline QStringList names {
      //% "Kilometers"
      QT_TRID_NOOP("qtnav-dist-units-km"),
      //% "Nautical miles"
      QT_TRID_NOOP("qtnav-dist-units-nm"),
      //% "Miles"
      QT_TRID_NOOP("qtnav-dist-units-mi"),
    };
  };

  static void setDistance(EnumDistance::type v) {
    self()->m_values["distance"] = static_cast<uint>(v);
  }
  static EnumDistance::type Distance() {
    return static_cast<EnumDistance::type>(self()->m_values["distance"].toUInt());
  }

  class EnumShortDistance {
  public:
    enum type {M, NM, Yds};
    static const inline QStringList names {
      //% "Meters"
      QT_TRID_NOOP("qtnav-sdist-units-m"),
      //% "Nautical miles"
      QT_TRID_NOOP("qtnav-sdist-units-nm"),
      //% "Yards"
      QT_TRID_NOOP("qtnav-sdist-units-yds"),
    };
  };

  static void setShortDistance(EnumShortDistance::type v) {
    self()->m_values["short_distance"] = static_cast<uint>(v);
  }
  static EnumShortDistance::type ShortDistance() {
    return static_cast<EnumShortDistance::type>(self()->m_values["short_distance"].toUInt());
  }

  class EnumBoatSpeed {
  public:
    enum type {Kn, Mph, Kmph};
    static const inline QStringList names {
      //% "Knots"
      QT_TRID_NOOP("qtnav-bspeed-units-kn"),
      //% "Mph"
      QT_TRID_NOOP("qtnav-bspeed-units-mph"),
      //% "Km/h"
      QT_TRID_NOOP("qtnav-bspeed-units-kmph"),
    };
  };

  static void setBoatSpeed(EnumBoatSpeed::type v) {
    self()->m_values["boat_speed"] = static_cast<uint>(v);
  }
  static EnumBoatSpeed::type BoatSpeed() {
    return static_cast<EnumBoatSpeed::type>(self()->m_values["boat_speed"].toUInt());
  }

private:

  Units();

};

}

