/* -*- coding: utf-8-unix -*-
 *
 * File: mobile/qml/TrackPointInfoBox.qml
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

TrackInfoBox {
  id: info

  property int index: 0

  target: Item {

    width: arrow.width + wp.width + padding
    height: wp.height + 2 * padding

    Image {
      id: arrow

      anchors {
        left: parent.left
        verticalCenter: parent.verticalCenter
      }

      x: padding
      height: 2.5 * padding
      width: height
      source: "icons/arrow.png"
    }

    Rectangle {

      id: wp

      anchors {
        left: arrow.right
        verticalCenter: parent.verticalCenter
        leftMargin: padding
        rightMargin: padding
      }

      height: label.height + padding / 2
      width: height
      radius: width / 2
      color: "#085efa"
      border.color: "white"
      border.width: padding / 4

      Text {
        id: label
        anchors.centerIn: parent
        text: "" + info.index
        color: "white"
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        font.pixelSize: info.fontSize * .6
        font.bold: true
      }
    }
  }
}
