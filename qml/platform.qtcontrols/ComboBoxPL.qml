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

import QtQuick 2.9
import QtQuick.Controls 2.2

Item {
  id: item
  anchors.left: parent.left
  anchors.leftMargin: theme.horizontalPageMargin
  anchors.right: parent.right
  anchors.rightMargin: theme.horizontalPageMargin
  height: Math.max(lab.height, val.height) + desc.height + desc.anchors.topMargin

  property int currentIndex
  property alias description: desc.text
  property alias model: val.model
  property alias label: lab.text
  property alias value: val.currentText

  Label {
    id: lab
    anchors.verticalCenter: val.verticalCenter
    anchors.left: parent.left
  }

  ComboBox {
    id: val
    anchors.left: lab.right
    anchors.leftMargin:theme.paddingMedium
    anchors.right: item.right
    anchors.top: parent.top
    font.pixelSize: theme.fontSizeMedium
    property bool initialized: false

    onCurrentIndexChanged: {
      if (initialized && currentIndex != item.currentIndex) {
        item.currentIndex = currentIndex;
      }
    }
    Component.onCompleted: {
      currentIndex = item.currentIndex;
      initialized = true;
    }
  }

  Label {
    id: desc
    anchors.left: parent.left
    anchors.right: parent.right
    anchors.top: val.bottom
    anchors.topMargin: text ? theme.paddingSmall : 0
    font.pixelSize: theme.fontSizeSmall
    height: text ? implicitHeight : 0
    visible: text
    wrapMode: Text.WordWrap
    font.italic: true
  }

  function activate() {
    // pass
  }
}
