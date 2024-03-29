/* -*- coding: utf-8-unix -*-
 *
 * File: mobile/qml/RouteButton.qml
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
  property bool editing

  onEditingChanged: {
    if (editing) {
      iconColor = "#214cad";
    } else {
      iconColor = "black";
    }
  }

  Component.onCompleted: {
    iconColor = "black";
    editing = false
  }


  anchors.bottom: parent.bottom
  anchors.bottomMargin: theme.paddingMedium
  anchors.rightMargin: theme.paddingMedium

  iconSource: app.getIcon("route")

  onClicked: {
    if (editing) {
      if (router.edited) {
        var dialog1 = app.show(Qt.resolvedUrl("RoutingResultDialog.qml"), {
                                 router: router
                               });
        dialog1.onAccepted.connect(function () {
          editing = false;
          tracker.reset();
        });
      } else {
        editing = false;
      }
    } else {
      if (!router.empty) {
        var dialog2 = app.show(Qt.resolvedUrl("RoutingStartDialog.qml"), {router: router});
        dialog2.onAccepted.connect(function () {
          editing = true;
        });
      } else {
        editing = true;
      }
    }
  }
}
