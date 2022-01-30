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

import QtQuick 2.6
import "./platform"

Column {

  id: root
  height: childrenRect.height
  width: parent.width
  spacing: theme.paddingMedium

  property alias description: desc.text
  property alias label: label.text
  property real value
  property string symbol
  property int precision: 1

  LabelPL {
    id: label
    font.pixelSize: theme.fontSizeMedium
    height: implicitHeight
    width: parent.width
    wrapMode: Text.WordWrap
    color: theme.primaryColor
  }

  Item {
    width: parent.width * .75 - theme.horizontalPageMargin
    height: val.height
    anchors.leftMargin: theme.horizontalPageMargin
    TextFieldPL {
      id: val
      anchors.bottomMargin: theme.paddingMedium
      height: implicitHeight
      width: parent.width - m.width
      font.pixelSize: theme.fontSizeMedium
      inputMethodHints: Qt.ImhFormattedNumbersOnly
      color: theme.highlightColor
      horizontalAlignment: TextInput.AlignHCenter
      Component.onCompleted: {
        text = root.value.toFixed(root.precision);
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
      anchors.leftMargin: theme.horizontalPageMargin
      text: root.symbol
      width: .5 * parent.width
      height: implicitHeight
    }
  }

  LabelPL {
    id: desc
    font.pixelSize: theme.fontSizeSmall
    font.italic: true
    height: text ? implicitHeight : 0
    width: parent.width
    visible: text
    wrapMode: Text.WordWrap
    color: theme.secondaryColor
  }

}
