/* -*- coding: utf-8-unix -*-
 *
 * File: PagePL.qml
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

Page {
  id: page

  property int pageHeight: 0 // dummy
  property string title
  default property alias content: body.data


  SilicaFlickable {
    id: flickable
    anchors.fill: parent
    contentHeight: header.height + 2 * Theme.paddingLarge + body.height

    PageHeader {
      id: header
      title: page.title
    }

    Column {
      id: body

      x: Theme.horizontalPageMargin
      width: parent.width - 2*x
      anchors {
        left: parent.left
        right: parent.right
        top: header.bottom
        leftMargin: Theme.horizontalPageMargin
      }
      spacing: Theme.paddingMedium
    }

    VerticalScrollDecorator {flickable: flickable}
  }

}
