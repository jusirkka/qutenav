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

  width: content.width + 2 * theme.paddingLarge
  height: content.height + 2 * theme.paddingLarge
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

  function show(msg, img, pos) {
    info.text = msg;
    fig.source = img ? img : ""
    state = pos ? pos : "top";
    timer.restart();
  }

  Row {
    id: content

    anchors.horizontalCenter: parent.horizontalCenter
    anchors.verticalCenter: parent.verticalCenter

    spacing: theme.paddingMedium

    Image {
      id: fig
      width: 32
      height: 32
      visible: source
    }

    Text {
      id: info
      color: "black"
      font.pixelSize: theme.fontSizeMedium
      horizontalAlignment: Text.AlignHCenter
    }
  }

  Timer {
    id: timer
    interval: 4000
    repeat: false
    running: false
  }
}
