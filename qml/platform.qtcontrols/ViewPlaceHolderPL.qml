/* -*- coding: utf-8-unix -*-
 *
 * File: ViewPlaceHolderPL.qml
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

import QtQuick 2.12
import QtQuick.Controls 2.12

Item {
  id: item
  anchors.fill: parent
  visible: enabled

  property string text
  property string hintText

  Label {
    id: main
    text: item.text
    anchors.centerIn: parent
    font.pixelSize: Theme.fontSizeLarge
    font.bold: true
  }

  Label {
    id: hint
    text: item.hintText
    anchors {
      top: main.bottom
      left: parent.left
      right: parent.right
    }
    font.pixelSize: Theme.fontSizeMedium
  }

}
