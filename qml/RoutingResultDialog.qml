/* -*- coding: utf-8-unix -*-
 *
 * File: mobile/qml/RoutingResultDialog.qml
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

  property var router
  property bool saveNeeded: true

  title: router.name()
  //% "Save and Show"
  acceptText: qsTrId("qutenav-dialog-route-result-save-show")

  onAccepted: {
    if (saveNeeded) {
      router.save();
    }
  }

  Column {
    spacing: theme.paddingMedium

    anchors {
      left: parent.left
      right: parent.right
    }

    width: parent.width

    IconListItemPL {
      //% "Save and Show"
      label: qsTrId("qutenav-dialog-route-result-save-show")
      iconName: app.getSystemIcon('save')
      onClicked: {
        dialog.accept();
      }
    }

    IconListItemPL {
      //% "Save and Close"
      label: qsTrId("qutenav-dialog-route-result-save-close")
      iconName: app.getSystemIcon('save')
      onClicked: {
        dialog.router.save();
        dialog.router.clear();
        dialog.saveNeeded = false;
        dialog.accept();
      }
    }

    IconListItemPL {
      //% "Close without saving"
      label: qsTrId("qutenav-dialog-route-result-close")
      iconName: app.getSystemIcon('clear')
      onClicked: {
        dialog.router.clear();
        dialog.saveNeeded = false;
        dialog.accept();
      }
    }
  }
}
