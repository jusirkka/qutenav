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

Dialog {
  id: page

  property int pageHeight: 300

  visible: true
  modal: true
  width: parent.width * 2 / 5
  height: pageHeight

  anchors.centerIn: parent

  default property alias content: body.data

  Flickable {
    id: view
    clip: true
    anchors.bottomMargin: theme.paddingLarge
    anchors.fill: parent
    anchors.centerIn: parent
    anchors.topMargin: theme.paddingLarge
    contentHeight: body.height
    contentWidth: parent.width - 2 * theme.paddingMedium

    ScrollBar.vertical: ScrollBar {
      visible: view.contentHeight > view.height
      padding: 0
      contentItem: Rectangle {
        opacity: 1
        color: "black"
        implicitWidth: theme.paddingMedium
        implicitHeight: 2 * theme.paddingMedium
      }
      background: Rectangle {
        opacity: 1
        color: "#e0dfd8"
        implicitHeight: view.height
        implicitWidth: theme.paddingMedium
      }
    }

    Column {
      id: body
      spacing: theme.paddingMedium
      anchors {
        left: parent.left
        right: parent.right
        bottom: parent.bottom
        leftMargin: theme.horizontalPageMargin
        rightMargin: theme.horizontalPageMargin
      }
    }
  }

  standardButtons: Dialog.Ok
}
