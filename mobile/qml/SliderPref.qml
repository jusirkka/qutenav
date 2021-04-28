/* -*- coding: utf-8-unix -*-
 *
 * Copyright (C) 2018 Rinigus
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

 Adapted to qutenav (C) 2021 Jukka Sirkka

 */

import QtQuick 2.0
import Sailfish.Silica 1.0

Item {

  height: val.height + desc.height + 2 * Theme.paddingMedium
  width: parent.width

  property alias description: desc.text
  property alias label: val.label
  property alias maximumValue: val.maximumValue
  property alias minimumValue: val.minimumValue
  property alias stepSize: val.stepSize
  property alias value: val.value

  Slider {
    id: val
    width: parent.width
    valueText: "" + value + "m"
  }

  Label {
    id: desc
    anchors.top: val.bottom
    anchors.topMargin: text ? Theme.paddingMedium : 0
    anchors.left: val.left
    anchors.right: val.right
    anchors.leftMargin: Theme.horizontalPageMargin
    anchors.rightMargin: Theme.horizontalPageMargin
    font.pixelSize: Theme.fontSizeSmall
    height: text ? implicitHeight : 0
    width: parent.width
    visible: text
    wrapMode: Text.WordWrap
    color: Theme.secondaryColor
  }
}
