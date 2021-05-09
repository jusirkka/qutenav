/* -*- coding: utf-8-unix -*-
 *
 * File: ValuePref.qml
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

import QtQuick 2.15

Column {

  id: root
  height: childrenRect.height
  width: parent.width
  spacing: Theme.paddingMedium

  property alias description: desc.text
  property alias label: label.text
  property real value

  LabelPL {
    id: label
    font.pixelSize: Theme.fontSizeMedium
    height: implicitHeight
    width: parent.width
    wrapMode: Text.WordWrap
    color: Theme.primaryColor
  }

  Item {
    width: parent.width * .75 - Theme.horizontalPageMargin
    height: m.height
    anchors.leftMargin: Theme.horizontalPageMargin
    TextFieldPL {
      id: val
      height: implicitHeight
      width: parent.width - m.width
      font.pixelSize: Theme.fontSizeMedium
      inputMethodHints: Qt.ImhFormattedNumbersOnly
      color: Theme.highlightColor
      horizontalAlignment: TextInput.AlignHCenter
      Component.onCompleted: {
        text = root.value.toFixed(1);
      }
      onFocusChanged: {
        if (!val.focus) {
          root.value = Number(val.text)
        }
      }
    }
    LabelPL {
      id: m
      anchors.left: val.right
      text: "meters"
      width: .5 * parent.width
      height: implicitHeight
    }
  }

  LabelPL {
    id: desc
    font.pixelSize: Theme.fontSizeSmall
    height: text ? implicitHeight : 0
    width: parent.width
    visible: text
    wrapMode: Text.WordWrap
    color: Theme.secondaryColor
  }

}
