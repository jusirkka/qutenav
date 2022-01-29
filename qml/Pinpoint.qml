/* -*- coding: utf-8-unix -*-
 *
 * File: qml/PinPoint.qml
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

Item {
  id: pin

  property point position
  property bool selected

  readonly property real sc: theme.paddingLarge

  signal clicked(bool down)

  onSelectedChanged: {
    z = selected ? 200 : 100
  }

  anchors.fill: parent

  // body
  Rectangle {
    id: body

    readonly property real dx: - pin.sc
    readonly property real dy: - pin.sc * 3

    x: pin.position.x
    y: pin.position.y - height / 2
    z: 50
    color: "black"
    height: 2
    width: Math.sqrt(dx * dx + dy * dy)
    transform: Rotation {
      origin.x: 0
      origin.y: body.height / 2
      angle: Math.atan2(body.dy, body.dx) * 180 / Math.PI
    }
  }

  // head + mousearea
  Rectangle {
    id: head

    width: (pin.selected ? 1.2 : 1) * 2 * pin.sc
    height: width
    radius: width / 2

    x: pin.position.x + body.dx - width / 2
    y: pin.position.y + body.dy - height / 2
    z: 100

    color: pin.selected ? "red" : "blue"
    border.color: pin.selected ? "black" : "white"

    MouseArea {
      id: mouse
      enabled: true
      anchors.fill: parent
      onClicked: {
        pin.selected = !pin.selected
        pin.clicked(pin.selected)
      }
    }
  }
}
