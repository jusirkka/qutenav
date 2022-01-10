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
import QtQuick 2.6
import "./platform"

DialogPL {
  id: dialog

  property var tracker: undefined
  property bool saveNeeded: true

  onAccepted: {
    if (saveNeeded) {
      console.log("saving track");
      tracker.save();
    }
  }

  title: "Stop tracking"
  acceptText: "Save"

  Column {
    spacing: theme.paddingMedium

    anchors {
      left: parent.left
      right: parent.right
    }

    width: parent.width


    IconListItemPL {
      label: "Save"
      iconName: app.getSystemIcon('save')
      onClicked: {
        dialog.accept();
      }
    }

    IconListItemPL {
      label: "Pause"
      iconName: app.getSystemIcon('pause')
      onClicked: {
        dialog.tracker.pause();
        dialog.saveNeeded = false;
        dialog.accept();
      }
    }

    IconListItemPL {
      label: "Delete"
      iconName: app.getSystemIcon('clear')
      onClicked: {
        dialog.tracker.remove();
        dialog.saveNeeded = false;
        dialog.accept();
      }
    }
  }
}

