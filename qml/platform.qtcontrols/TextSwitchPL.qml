/* -*- coding: utf-8-unix -*-
 *
 * Copyright (C) 2018 Rinigus
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

import QtQuick 2.15
import QtQuick.Controls 2.12

Item {
  id: item
  height: sw.height + desc.height + desc.anchors.topMargin
  width: parent.width

  property alias checked: sw.checked
  property alias description: desc.text
  property alias text: sw.text

  Switch {
    id: sw
    anchors.left: parent.left
    anchors.leftMargin: theme.horizontalPageMargin
    anchors.right: parent.right
    anchors.rightMargin: theme.horizontalPageMargin
    anchors.top: parent.top
    font.pixelSize: theme.fontSizeMedium
  }

  Label {
    id: desc
    anchors.left: parent.left
    anchors.leftMargin: theme.horizontalPageMargin
    anchors.right: parent.right
    anchors.rightMargin: theme.horizontalPageMargin
    anchors.top: sw.bottom
    anchors.topMargin: text ? theme.paddingSmall : 0
    font.pixelSize: theme.fontSizeSmall
    font.italic: true
    height: text ? implicitHeight : 0
    visible: text
    wrapMode: Text.WordWrap
  }
}
