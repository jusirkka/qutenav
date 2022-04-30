/* -*- coding: utf-8-unix -*-
 *
 * File: mobile/qml/RoutingStartDialog.qml
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

  //% "Route editor"
  title: qsTrId("qutenav-dialog-route-editor")
  //% "Edit current"
  acceptText: qsTrId("qutenav-dialog-edit-current")

  Column {
    spacing: theme.paddingMedium

    anchors {
      left: parent.left
      right: parent.right
    }

    width: parent.width

    IconListItemPL {
      //% "Edit current"
      label: qsTrId("qutenav-dialog-edit-current")
      iconName: app.getSystemIcon('edit')
      onClicked: {
        dialog.accept();
      }
    }

    IconListItemPL {
      //% "Create new route"
      label: qsTrId("qutenav-dialog-create-new-route")
      iconName: app.getSystemIcon('new')
      onClicked: {
        dialog.router.clear();
        dialog.accept();
      }
    }

    IconListItemPL {
      //% "Close current"
      label: qsTrId("qutenav-dialog-create-close-current")
      iconName: app.getSystemIcon('clear')
      onClicked: {
        dialog.router.clear();
        dialog.reject();
      }
    }
  }
}
