/* -*- coding: utf-8-unix -*-
 *
 * File: mobile/qml/ValuePref.qml
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
import QtQuick 2.2
import Sailfish.Silica 1.0

Column {

  id: root
  height: childrenRect.height
  width: parent.width
  spacing: Theme.paddingMedium

  property alias description: desc.text
  property alias label: label.text
  property real value

  Label {
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
    TextField {
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
    Label {
      id: m
      anchors.left: val.right
      text: "meters"
      width: .5 * parent.width
      height: implicitHeight
    }
  }

  Label {
    id: desc
    font.pixelSize: Theme.fontSizeSmall
    height: text ? implicitHeight : 0
    width: parent.width
    visible: text
    wrapMode: Text.WordWrap
    color: Theme.secondaryColor
  }

}
