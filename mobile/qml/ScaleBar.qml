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

import QtQuick 2.2
import Sailfish.Silica 1.0


Item {
  id: scaleBar
  anchors.bottomMargin: Theme.paddingMedium
  anchors.right: parent.right
  anchors.rightMargin: Theme.paddingMedium
  opacity: 0.9
  visible: scaleWidth > 0

  implicitWidth: base.width

  property real scaleWidth: 0
  property string text: ""

  Rectangle {
    id: base
    color: "black"
    height: Math.floor(Theme.pixelRatio * 3)
    width: scaleBar.scaleWidth
  }

  Rectangle {
    anchors.bottom: base.top
    anchors.left: base.left
    color: "black"
    height: Math.floor(Theme.pixelRatio * 10)
    width: Math.floor(Theme.pixelRatio * 3)
  }

  Rectangle {
    anchors.bottom: base.top
    anchors.right: base.right
    color: "black"
    height: Math.floor(Theme.pixelRatio * 10)
    width: Math.floor(Theme.pixelRatio * 3)
  }

  Text {
    anchors.bottom: base.top
    anchors.bottomMargin: Math.floor(Theme.pixelRatio * 4)
    anchors.horizontalCenter: base.horizontalCenter
    color: "black"
    font.bold: true
    font.family: "sans-serif"
    font.pixelSize: Math.round(Theme.pixelRatio * 18)
    horizontalAlignment: Text.AlignHCenter
    text: scaleBar.text
  }
}
