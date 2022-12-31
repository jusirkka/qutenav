/* -*- coding: utf-8-unix -*-
 *
 * File: mobile/qml/MenuButton.qml
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

  relativeMargin: 1.1
  iconSize: 48

  anchors {
    bottom: parent.bottom
    bottomMargin: theme.paddingMedium
    leftMargin: theme.paddingMedium
  }

  iconColor: "transparent"
  iconSource: app.getIcon("query")


  onClicked: {
    if (!infoQueryPending) {
      infoQueryPending = true
      infoPendingTimer.restart()
      encdis.infoQuery(infoPoint.peepHole, true)
    }
  }
}
