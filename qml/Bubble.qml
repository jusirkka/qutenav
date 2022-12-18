/* -*- coding: utf-8-unix -*-
 *
 * File: qml/Bubble.qml
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

  width: bubbleContent.width + 2 * theme.paddingLarge
  height: bubbleContent.height + 2 * theme.paddingLarge
  radius: 20
  color: "white"
  border.color: "black"

  property var topItem: null
  property var bottomItem: null
  property alias target: bubbleContent.sourceComponent

  visible: timer.running

  function _restart(msecs) {
    timer.interval = msecs
    timer.restart()
  }

  state: "top"

  states: [
    State {
      name: "bottom"
      AnchorChanges {
        anchors.bottom: bottomItem ? bottomItem.top : parent.bottom
        target: item
      }
    },
    State {
      name: "top"
      AnchorChanges {
        anchors.top: topItem ? topItem.bottom : parent.top
        target: item
      }
    },
    State {
      name: "center"
      AnchorChanges {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.verticalCenter: parent.verticalCenter
        target: item
      }
    }
  ]

  Timer {
    id: timer
    interval: 4000
    repeat: false
    running: false
  }

  Loader {
    id: bubbleContent

    anchors.horizontalCenter: parent.horizontalCenter
    anchors.verticalCenter: parent.verticalCenter
  }
}
