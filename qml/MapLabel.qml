/* -*- coding: utf-8-unix -*-
 *
 * File: qml/Maplabel.qml
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

Rectangle {
  id: rect

  property string label

  width: label.width + 2 * theme.paddingLarge
  height: label.height + 2 * theme.paddingMedium
  radius: .15 * height
  color: "#214cad"
  border.color: "black"

  anchors {
    bottomMargin: theme.paddingMedium
    leftMargin: theme.paddingMedium
    rightMargin: theme.paddingMedium
  }

  Text {
    id: label
    anchors.centerIn: parent
    text: rect.label
    color: "white"
    horizontalAlignment: Text.AlignHCenter
    verticalAlignment: Text.AlignVCenter
    font.pixelSize: theme.fontSizeSmall
    font.bold: true
  }
}
