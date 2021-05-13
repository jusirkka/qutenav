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
import QtQuick 2.6

Rectangle {

  id: item

  anchors.topMargin: theme.paddingLarge
  anchors.bottomMargin: theme.paddingLarge
  anchors.horizontalCenter: parent.horizontalCenter

  width: info.width + 2 * theme.paddingLarge
  height: info.height + 2 * theme.paddingLarge
  radius: 20
  color: "white"
  border.color: "black"

  visible: timer.running

  state: "top"

  states: [
    State {
      name: "bottom"
      AnchorChanges {
        anchors.bottom: menuButton.top
        target: item
      }
    },
    State {
      name: "top"
      AnchorChanges {
        anchors.top: trackInfo.bottom
        target: item
      }
    }
  ]

  property int tics: 16

  function show(msg, pos) {
    info.text = msg;
    timer.restart();
    state = pos;
  }

  Text {
    id: info
    color: "black"
    anchors.horizontalCenter: parent.horizontalCenter
    anchors.verticalCenter: parent.verticalCenter
    font.pixelSize: theme.fontSizeMedium
  }

  Timer {
    id: timer
    interval: 4000
    repeat: false
    running: false
  }
}
