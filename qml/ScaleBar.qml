/* -*- coding: utf-8-unix -*-
 *
 * Copyright (C) 2014 Osmo Salomaa
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
  id: scaleBar
  anchors.bottomMargin: theme.paddingMedium
  anchors.right: parent.right
  anchors.rightMargin: theme.paddingMedium
  opacity: 0.9
  visible: scaleWidth > 0

  implicitWidth: base.width

  property real scaleWidth: 0
  property string distanceText: ""
  property string scaleText: ""
  property string scaleText2: ""

  Rectangle {
    id: base
    color: "black"
    height: Math.floor(theme.pixelRatio * 3)
    width: scaleBar.scaleWidth
  }

  Rectangle {
    id: lbar
    anchors.bottom: base.top
    anchors.left: base.left
    color: "black"
    height: Math.floor(theme.pixelRatio * 10)
    width: Math.floor(theme.pixelRatio * 3)
  }

  Rectangle {
    id: rbar
    anchors.bottom: base.top
    anchors.right: base.right
    color: "black"
    height: Math.floor(theme.pixelRatio * 10)
    width: Math.floor(theme.pixelRatio * 3)
  }

  Text {
    anchors.bottom: base.top
    anchors.bottomMargin: Math.floor(theme.pixelRatio * 4)
    anchors.rightMargin: theme.paddingSmall
    anchors.right: rbar.left
    color: "black"
    font.bold: true
    font.family: "sans-serif"
    font.pixelSize: Math.round(theme.pixelRatio * 18)
    horizontalAlignment: Text.AlignHRight
    text: scaleBar.distanceText
  }

  Text {
    anchors.bottom: base.top
    anchors.bottomMargin: Math.floor(theme.pixelRatio * 4)
    anchors.leftMargin: theme.paddingSmall
    anchors.left: lbar.right
    color: "black"
    font.bold: true
    font.family: "sans-serif"
    font.pixelSize: Math.round(theme.pixelRatio * 18)
    horizontalAlignment: Text.AlignHLeft
    text: scaleBar.scaleText
  }

  Text {
    anchors.top: base.bottom
    anchors.topMargin: Math.floor(theme.pixelRatio * 4)
    anchors.leftMargin: theme.paddingSmall
    anchors.left: lbar.right
    color: "green"
    font.bold: true
    font.family: "sans-serif"
    font.pixelSize: Math.round(theme.pixelRatio * 18)
    horizontalAlignment: Text.AlignHLeft
    text: scaleBar.scaleText2
    visible: text.length > 0
  }
}
