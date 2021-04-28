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
import QtQuick 2.0
import Sailfish.Silica 1.0

Dialog {
  id: dialog

  property var router: undefined
  property bool saveNeeded: true

  onAccepted: {
    if (saveNeeded) {
      router.save();
    }
  }

  SilicaFlickable {
    id: flickable
    anchors.fill: parent

    Column {
      anchors.fill: parent

      DialogHeader {
        title: router.name()
        acceptText: "Save & Show"
        cancelText: "Cancel"
      }

      IconListItem {
        label: "Save & Show"
        icon: "image://theme/icon-m-device-upload"
        onClicked: {
          dialog.accept();
        }
      }

      IconListItem {
        label: "Save & Close"
        icon: "image://theme/icon-m-device-upload"
        onClicked: {
          dialog.router.save();
          dialog.router.clear();
          dialog.saveNeeded = false;
          dialog.accept();
        }
      }


      IconListItem {
        label: "Close without saving"
        icon: "image://theme/icon-m-clear"
        onClicked: {
          dialog.router.clear();
          dialog.saveNeeded = false;
          dialog.accept();
        }
      }
    }

    VerticalScrollDecorator {flickable: flickable}
  }
}
