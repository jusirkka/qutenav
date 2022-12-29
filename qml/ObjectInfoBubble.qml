/* -*- coding: utf-8-unix -*-
 *
 * File: qml/ObjectInfoBubble.qml
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

  id: objectInfo

  property string imgSource
  property string text

  function show(msg, img, pos) {
    text = msg
    imgSource = img
    state = pos ? pos : "top"
    _restart(4000)
  }

  target: Item {
    id: content

    height: Math.max(fig.height, info.height)
    width: fig.width + info.width + 2 * theme.paddingSmall

    Image {
      id: fig
      cache: false
      source: objectInfo.imgSource
    }

    TextMetrics {
      id: maxMetrics
      text: "sample text for measurement of max line len"
      font: info.font
    }

    Text {
      id: info
      width: Math.min(textWidth, maxMetrics.boundingRect.width) + theme.paddingSmall

      readonly property alias textWidth: bubbleMetrics.boundingRect.width

      anchors {
        right: parent.right
        verticalCenter: content.verticalCenter
      }
      color: "black"
      font.pixelSize: theme.fontSizeMedium
      horizontalAlignment: Text.AlignLeft
      wrapMode: Text.WordWrap

      text: objectInfo.text

      TextMetrics {
        id: bubbleMetrics
        text: info.text
        font: info.font
      }
    }
  }
}
