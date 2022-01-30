/* -*- coding: utf-8-unix -*-
 *
 * Copyright (C) 2022 Jukka Sirkka
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

import QtQuick 2.6
import "./platform"

PagePL {

  //% "System"
  title: qsTrId("qtnav-title-system")
  pageHeight: parent.height

  SectionHeaderPL {
    //% "Cache"
    text: qsTrId("qtnav-header-cache")
  }

  ValuePref {
    precision: 0
    //% "Size"
    label: qsTrId("qtnav-system-cache-size")
    //% "Charts are cached. The cache files are deleted starting from the oldest if the cache grows larger than this value."
    description: qsTrId("qtnav-system-cache-description")
    //% "MB"
    symbol: qsTrId("qtnav-unit-mb-si")
    Component.onCompleted: {
      value = settings.cacheSize
    }
    onValueChanged: {
      settings.cacheSize = Math.floor(value)
    }
  }

}
