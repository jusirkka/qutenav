/* -*- coding: utf-8-unix -*-
 *
 * File: qml/SimpleBubble.qml
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

Bubble {

  id: simple

  property string text

  function show(msg, pos) {
    text = msg
    state = pos ? pos : "top"
    _restart(4000)
  }

  function notify(msg) {
    text = msg
    state = "center"
    _restart(1500)
  }

  target: Item {
    id: content

    height: info.height
    width: info.width + 2 * theme.paddingSmall

    Text {
      id: info

      text: simple.text

      anchors {
        right: parent.right
        verticalCenter: content.verticalCenter
      }
      color: "black"
      font.pixelSize: theme.bubbleTextFontSize
      horizontalAlignment: Text.AlignLeft
    }
  }
}
