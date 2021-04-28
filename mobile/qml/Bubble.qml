/* -*- coding: utf-8-unix -*-
 *
 * File: mobile/qml/Bubble.qml
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
import QtQuick 2.0
import Sailfish.Silica 1.0

Rectangle {

  id: item

  anchors.topMargin: Theme.paddingLarge
  anchors.bottomMargin: Theme.paddingLarge
  anchors.horizontalCenter: parent.horizontalCenter

  width: info.width + 2 * Theme.paddingLarge
  height: info.height + 2 * Theme.paddingLarge
  radius: 20
  color: "white"
  border.color: "black"

  visible: timer.running

  state: "top"

  states: [
    State {
      name: "bottom"
      AnchorChanges {
        anchors.bottom: parent.bottom
        target: item
      }
    },
    State {
      name: "top"
      AnchorChanges {
        anchors.top: parent.top
        target: item
      }
    }
  ]

  property int tics: 16

  function show(msg, pos) {
    info.text = msg;
    timer.start();
    state = pos;
  }

  Label {
    id: info
    color: "black"
    anchors.horizontalCenter: parent.horizontalCenter
    anchors.verticalCenter: parent.verticalCenter
    font.pixelSize: Theme.fontSizeMedium
  }

  Timer {
    id: timer
    interval: 4000
    repeat: false
    running: false
  }
}
