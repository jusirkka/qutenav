/* -*- coding: utf-8-unix -*-
 *
 * File: DialogPL.qml
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
import Sailfish.Silica 1.0
import org.qutenav 1.0

Dialog {
  id: dialog

  property string title
  property string acceptText: "Accept"
  property int pageHeight // dummy
  property int pageWidth // dummy
  property bool hasOK: false // dummy


  default property alias content: body.data

  SilicaFlickable {
    id: flickable
    anchors.fill: parent

    DialogHeader {
      id: header
      title: dialog.title
      acceptText: dialog.acceptText
      cancelText: "Cancel"
    }

    Item {
      id: body

      anchors {
        top: header.bottom
        bottom: parent.bottom
        left: parent.left
        right: parent.right
        topMargin: theme.paddingLarge
      }
    }
    VerticalScrollDecorator {flickable: flickable}
  }
}
