/* -*- coding: utf-8-unix -*-
 *
 * File: mobile/qml/IconListItem.qml
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

ListItem {
  id: item
  anchors.left: parent.left
  anchors.right: parent.right
  contentHeight: theme.itemSizeSmall

  property string label: ""
  property string iconName: ""

  Image {
    id: icon
    fillMode: Image.PreserveAspectFit
    anchors.left: parent.left
    anchors.leftMargin: theme.horizontalPageMargin
    anchors.verticalCenter: parent.verticalCenter
    sourceSize.height: theme.itemSizeSmall * 0.8
    source: item.iconName
  }

  Label {
    id: label
    anchors.left: icon.right
    anchors.leftMargin: theme.paddingMedium
    anchors.right: parent.right
    anchors.rightMargin:theme.horizontalPageMargin
    height: theme.itemSizeSmall
    text: item.label
    verticalAlignment: Text.AlignVCenter
  }

}
