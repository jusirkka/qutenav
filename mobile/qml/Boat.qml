/* -*- coding: utf-8-unix -*-
 *
 * File: mobile/qml/Boat.qml
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

Item {

  id: item

  height: img.height
  width: img.width

  property point center
  property point heading
  property bool hasHeading

  x: center.x - img.width / 2
  y: center.y - img.height / 2

  Image {
    id: img
    smooth: false
    visible: true
    source: app.getIcon("ship")
  }

  Rectangle {
    id: head
    x: img.width / 2
    y: img.height / 2 - height / 2
    visible: item.hasHeading
    color: "red"
    height: 4
    width: Math.sqrt(item.heading.x * item.heading.x + item.heading.y * item.heading.y)
    transform: Rotation {
      origin.x: 0
      origin.y: head.height / 2
      angle: Math.atan2(item.heading.y, item.heading.x) * 180 / Math.PI
    }
  }

}
