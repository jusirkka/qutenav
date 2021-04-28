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
import Sailfish.Silica 1.0

Rectangle {
  id: rect

  property bool tracking

  height: button.height * 1.45
  width: height
  radius: 180
  color: "white"
  border.color: "black"

  anchors.bottom: parent.bottom
  anchors.bottomMargin: Theme.paddingMedium
  anchors.leftMargin: Theme.paddingMedium

  onTrackingChanged: {
    if (tracking) {
      button.icon.color = "#ff0000";
    } else {
      button.icon.color = "black";
    }
  }

  IconButton {
    id: button
    anchors.verticalCenter: parent.verticalCenter
    anchors.horizontalCenter: parent.horizontalCenter

    height: icon.sourceSize.height
    icon.smooth: false
    icon.source: app.getIcon("record");
    icon.color: "black"

    onClicked: {
      if (rect.tracking) {
        var dialog = pageStack.push(Qt.resolvedUrl("TrackResultDialog.qml"), {tracker: tracker});
        dialog.onAccepted.connect(function () {
          console.log("stop tracking");
          rect.tracking = false;
        });
      } else {
        console.log("keep tracking");
        rect.tracking = true;
        tracker.start();
      }
      tracker.sync();
    }
  }
}
