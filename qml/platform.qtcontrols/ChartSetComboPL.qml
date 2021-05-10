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

 Adapted to qutenav (c) 2021 Jukka Sirkka, same licence as above

 */

import QtQuick 2.15
import QtQuick.Controls 2.12

Item {
  id: item

  anchors.left: parent.left
  anchors.right: parent.right
  height: theme.itemSizeMedium
  width: 300

  Label {
    id: label
    anchors.left: parent.left
    text: "Chartsets"
  }

  ComboBox {
    id: box
    anchors.left: label.right
    anchors.right: parent.right
    anchors.leftMargin: theme.horizontalPageMargin

    onCurrentIndexChanged: {
      encdis.chartSet = model[currentIndex];
    }
  }

  Component.onCompleted: {
    box.model = encdis.chartSets;
    box.currentIndex = 0;
    var active = encdis.chartSet;
    for (var i = 0; i < box.model.length; i++) {
      if (box.model[i] === active) {
        box.currentIndex = i;
        break;
      }
    }
  }
}
