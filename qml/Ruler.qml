/* -*- coding: utf-8-unix -*-
 *
 * File: qml/Ruler.qml
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
import QtPositioning 5.2

Item {
  id: item

  property point delta
  property int selection
  property var positionChangeHandler
  property bool measuring

  property var c1: QtPositioning.coordinate(0, 0)
  property var c2: QtPositioning.coordinate(0, 0)

  function sync() {
    p1.position = encdis.position(c1)
    p2.position = encdis.position(c2)
  }

  function moveP1(d) {p1.position.x += d.x; p1.position.y += d.y; c1 = encdis.tocoord(p1.position)}
  function moveP2(d) {p2.position.x += d.x; p2.position.y += d.y; c2 = encdis.tocoord(p2.position)}
  function noop(d) {}

  function selection1(down) {
    if (down) {
      if (selection === 2) {
        p2.selected = false
      }
      selection = 1
      positionChangeHandler = moveP1
    } else {
      if (selection === 1) {
        selection = 0
        positionChangeHandler = noop
      }
    }
    console.log("p1 pos", c1)
    console.log("p2 pos", c2)
  }

  function selection2(down) {
    if (down) {
      if (selection === 1) {
        p1.selected = false
      }
      selection = 2
      positionChangeHandler = moveP2
    } else {
      if (selection === 2) {
        selection = 0
        positionChangeHandler = noop
      }
    }
    console.log("p1 pos", c1)
    console.log("p2 pos", c2)
  }

  onMeasuringChanged: {
    p1.position.x = .45 * width; p1.position.y = .5 * height
    p2.position.x = .55 * width; p2.position.y = .5 * height
    c1 = encdis.tocoord(p1.position)
    c2 = encdis.tocoord(p2.position)
  }

  onDeltaChanged: {
    positionChangeHandler(delta)
  }

  Component.onCompleted: {
    selection = 0
    positionChangeHandler = noop
    p1.clicked.connect(selection1)
    p2.clicked.connect(selection2)
  }

  anchors.fill: parent

  Pinpoint {
    id: p1
    z: 100
  }

  Pinpoint {
    id: p2
    z: 100
  }

  Rectangle {
    id: line
    z: 50

    property real dx: p2.position.x - p1.position.x
    property real dy: p2.position.y - p1.position.y

    x: p1.position.x
    y: p1.position.y - height / 2
    color: "cyan"
    height: 4
    width: Math.sqrt(dx * dx + dy * dy)
    transform: Rotation {
      origin.x: 0
      origin.y: line.height / 2
      angle: Math.atan2(line.dy, line.dx) * 180 / Math.PI
    }
  }
}
