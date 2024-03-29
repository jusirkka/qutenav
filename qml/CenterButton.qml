/* -*- coding: utf-8-unix -*-
 *
 * File: qml/CenterButton.qml
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

MapButtonPL {

  id: button

  property bool centered

  anchors.bottom: parent.bottom
  anchors.bottomMargin: theme.paddingMedium
  anchors.rightMargin: theme.paddingMedium

  onCenteredChanged: {
    if (centered) {
      iconColor = "#00b000";
    } else {
      iconColor = "black";
    }
  }

  iconSource: app.getIcon("center")


  onClicked: {
    centered = true;
  }
}
