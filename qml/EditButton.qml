/* -*- coding: utf-8-unix -*-
 *
 * File: mobile/qml/EditButton.qml
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
import QtQuick 2.2

Rectangle {
  id: rect

  property string label

  signal clicked

  width: label.width + 2 * Theme.paddingLarge
  height: label.height + 2 * Theme.paddingMedium
  radius: .4 * height
  color: "#214cad"
  border.color: "black"

  anchors.bottomMargin: Theme.paddingMedium
  anchors.leftMargin: Theme.paddingMedium
  anchors.rightMargin: Theme.paddingMedium


  Text {
    id: label
    anchors.centerIn: parent
    text: rect.label
    color: "white"
    horizontalAlignment: Text.AlignHCenter
    verticalAlignment: Text.AlignVCenter
    font.pixelSize: Theme.fontSizeSmall
    font.bold: true
  }

  MouseArea {
    id: mouse
    anchors.fill: parent

    onClicked: {
      rect.clicked();
    }
  }

}
