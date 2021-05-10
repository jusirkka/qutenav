/* -*- coding: utf-8-unix -*-
 *
 * File: mobile/qml/DimensionalValue.qml
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
import QtQuick 2.6

Item {

  id: item

  property real value
  property int fontSize
  property string unit

  implicitHeight: left.implicitHeight
  implicitWidth: left.implicitWidth + rbot.implicitWidth

  Text {
    id: left
    text: "" + (!isNaN(item.value) ? Math.floor(item.value) : "-")
    font.pixelSize: item.fontSize
    font.family: "Arial Black"
  }

  Text {
    id: rbot
    anchors.left: left.right
    anchors.baseline: left.baseline
    text: "." + (!isNaN(item.value) ?
                   Math.floor(10 * (item.value - Math.floor(item.value))) : "-")
    font.pixelSize: item.fontSize / 2
    font.family: "Arial Black"
  }

  Text {
    id: rtop
    anchors.left: left.right
    anchors.baseline: rbot.top
    text: item.unit
    font.pixelSize: item.fontSize / 3
    font.family: "Arial Black"
  }
}
