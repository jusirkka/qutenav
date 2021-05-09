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
import QtQuick 2.2

MapButton {
  id: button
  property bool tracking

  anchors.bottom: parent.bottom
  anchors.bottomMargin: Theme.paddingMedium
  anchors.leftMargin: Theme.paddingMedium

  Component.onCompleted: {
    icon.color = "black";
    tracking = false
  }

  onTrackingChanged: {
    if (tracking) {
      icon.color = "#ff0000";
    } else {
      icon.color = "black";
    }
  }

  icon.source: app.getIcon("record")

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
