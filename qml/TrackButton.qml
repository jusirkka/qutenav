/* -*- coding: utf-8-unix -*-
 *
 * File: mobile/qml/TrackButton.qml
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

MapButtonPL {

  id: button
  property bool tracking

  anchors.bottom: parent.bottom
  anchors.bottomMargin: theme.paddingMedium
  anchors.leftMargin: theme.paddingMedium

  Component.onCompleted: {
    iconColor = "black";
    tracking = false
  }

  onTrackingChanged: {
    if (tracking) {
      iconColor = "#ff0000";
    } else {
      iconColor = "black";
    }
  }

  iconSource: app.getIcon("record")

  onClicked: {
    if (tracking) {
      var dialog = app.show(Qt.resolvedUrl("TrackResultDialog.qml"), {tracker: tracker});
      dialog.onAccepted.connect(function () {
        console.log("stop tracking");
        tracking = false;
      });
    } else {
      console.log("keep tracking");
      tracking = true;
      tracker.start();
    }
    tracker.sync();
  }

}
