/* -*- coding: utf-8-unix -*-
 *
 * File: MapButton.qml
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

import QtQuick 2.15
import QtQuick.Controls 2.12

Button {
  id: item

  property string iconSource
  property string iconColor: "transparent"
  property real relativeMargin: 1.45
  property int iconSize: 24

  height: icon.height * relativeMargin
  width: height

  icon.color: iconColor
  icon.source: iconSource
  icon.height: iconSize
  icon.width: icon.height
  padding: 0

  display: AbstractButton.IconOnly

  background: Rectangle {
    height: icon.height * relativeMargin
    width: height
    radius: height / 2
    opacity: enabled ? 1 : 0.3
    color: "white"
    border.color: "black"
  }
}


