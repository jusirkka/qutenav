/* -*- coding: utf-8-unix -*-
 *
 * File: mobile/qml/RoutePoint.qml
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

Rectangle {
  id: rect

  z: 250

  property int index
  property point center
  property bool selected: false
  property bool editMode: true

  x: center.x - width / 2
  y: center.y - height / 2

  signal clicked(var rp)

  onSelectedChanged: {
    clicked(rect)
  }

  width: (rect.editMode ? (rect.selected ? 4 : 3) : 2)  * theme.paddingLarge
  height: width
  radius: width / 2
  color: rect.editMode ? (rect.selected ? "cyan" : "white") : "#085efa"
  border.color: rect.editMode ? "black" : "white"

  Text {
    id: label
    anchors.centerIn: parent
    text: "" + index
    color: rect.editMode ? "black" : "white"
    horizontalAlignment: Text.AlignHCenter
    verticalAlignment: Text.AlignVCenter
    font.pixelSize: rect.editMode ? theme.fontSizeSmall : theme.fontSizeExtraSmall
    font.bold: true
  }

  MouseArea {
    id: mouse
    anchors.fill: parent
    onClicked: {
      rect.selected = !rect.selected;
    }
    enabled: rect.editMode
  }

}
