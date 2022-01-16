/* -*- coding: utf-8-unix -*-
 *
 * File: ButtonPL.qml
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
import Sailfish.Silica 1.0
import org.qutenav 1.0
import QtQuick 2.6

BackgroundItem {
  id: item
  width: link.width + theme.paddingSmall
  height: link.height

  property alias text: link.text
  signal linkActivated()

  onClicked: {
    linkActivated()
  }

  Text {
    id: link
    color: item.highlighted ? theme.highlightColor : theme.primaryColor
    font.underline: true
    font.pixelSize: theme.fontSizeSmall
    textFormat: Text.PlainText
    wrapMode: Text.Wrap
  }

}
