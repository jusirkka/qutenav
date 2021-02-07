/* -*- coding: utf-8-unix -*-
 *
 * Copyright (C) 2018-2019 Rinigus, 2019 Purism SPC
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

 Adapted to qopencpn (c) 2021 Jukka Sirkka, same licence as above

 */

import QtQuick 2.2
import Sailfish.Silica 1.0

ListItem {
  id: item

  anchors.left: parent.left
  anchors.right: parent.right
  contentHeight: Theme.itemSizeSmall

  property string label: ""
  property string icon: ""
  property alias iconHeight: icon.sourceSize.height

  Image {
    id: icon
    fillMode: Image.PreserveAspectFit
    anchors.left: parent.left
    anchors.leftMargin: Theme.horizontalPageMargin
    anchors.verticalCenter: parent.verticalCenter
    sourceSize.height: Theme.itemSizeSmall * 0.8
    source: item.icon
  }

  Label {
    id: label
    anchors.left: icon.right
    anchors.leftMargin: Theme.paddingMedium
    anchors.rightMargin:Theme.horizontalPageMargin
    height: Theme.itemSizeSmall
    text: item.label
    verticalAlignment: Text.AlignVCenter
  }

  ComboBox {
    id: box
    anchors.left: label.right
    anchors.leftMargin: Theme.paddingMedium
    anchors.right: parent.right
    anchors.top: parent.top

    menu: ContextMenu {
      Repeater {
        model: box.model.length
        MenuItem {text: box.model[index]}
      }
    }

    property var model

    function activate() {
      if (!box.menu.active) box.menu.open(box)
    }

    onCurrentIndexChanged: {
      encdis.chartSet = model[box.currentIndex];
    }
  }

  Component.onCompleted: {
    box.currentIndex = 0;
    box.model = encdis.chartSets;
    var active = encdis.chartSet;
    for (var i = 0; i < box.model.length; i++) {
      if (box.model[i] === active) {
        box.currentIndex = i;
        break;
      }
    }
  }

  onClicked: box.activate()
}
