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

import QtQuick 2.15

DialogPL {
  id: dialog

  property var router: undefined
  property bool saveNeeded: true

  title: router.name()
  acceptText: "Save & Show"

  onAccepted: {
    if (saveNeeded) {
      router.save();
    }
  }

  Column {
    spacing: Theme.paddingMedium

    anchors {
      left: parent.left
      right: parent.right
    }

    width: parent.width

    IconListItemPL {
      label: "Save & Show"
      iconName: app.getSystemIcon('save')
      onClicked: {
        dialog.accept();
      }
    }

    IconListItemPL {
      label: "Save & Close"
      iconName: app.getSystemIcon('save')
      onClicked: {
        dialog.router.save();
        dialog.router.clear();
        dialog.saveNeeded = false;
        dialog.accept();
      }
    }

    IconListItemPL {
      label: "Close without saving"
      iconName: app.getSystemIcon('clear')
      onClicked: {
        dialog.router.clear();
        dialog.saveNeeded = false;
        dialog.accept();
      }
    }
  }
}
