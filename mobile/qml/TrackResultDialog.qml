/* -*- coding: utf-8-unix -*-
 *
 * File: mobile/qml/TrackResultDialog.qml
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
import QtQuick 2.0
import Sailfish.Silica 1.0

Dialog {
  id: dialog

  property var tracker: undefined
  property bool saveNeeded: true

  onAccepted: {
    if (saveNeeded) {
      console.log("saving track");
      tracker.save();
    }
  }

  SilicaFlickable {
    id: flickable
    anchors.fill: parent

    Column {
      anchors.fill: parent

      DialogHeader {
        title: "Stop tracking"
        acceptText: "Save"
        cancelText: "Cancel"
      }

      IconListItem {
        label: "Save"
        icon: "image://theme/icon-m-device-upload"
        onClicked: {
          dialog.accept();
        }
      }

      IconListItem {
        label: "Pause"
        icon: "image://theme/icon-m-pause"
        onClicked: {
          dialog.tracker.pause();
          dialog.saveNeeded = false;
          dialog.accept();
        }
      }

      IconListItem {
        label: "Delete"
        icon: "image://theme/icon-m-clear"
        onClicked: {
          dialog.tracker.remove();
          dialog.saveNeeded = false;
          dialog.accept();
        }
      }
    }

    VerticalScrollDecorator {flickable: flickable}
  }
}
