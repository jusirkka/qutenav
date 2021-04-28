/* -*- coding: utf-8-unix -*-
 *
 * File: mobile/qml/RouteDisplayDialog.qml
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
import org.qutenav 1.0

Dialog {
  id: dialog

  property var router: undefined
  property int routeId: -1

  onAccepted: {
    if (routeId > 0) {
      router.load(routeId);
    }
  }

  SilicaFlickable {
    id: flickable
    anchors.fill: parent

    DialogHeader {
      id: header
      title: "Select route"
      acceptText: "Most Recent"
      cancelText: "Cancel"
    }

    SilicaListView {
      id: routes

      anchors {
        top: header.bottom
        bottom: parent.bottom
        left: parent.left
        right: parent.right
        topMargin: Theme.paddingLarge
      }

      model: RouteModel {}

      delegate: ListItem {
        contentHeight: label.height + 2 * Theme.paddingMedium

        Label {
          id: label
          anchors.left: parent.left
          anchors.right: parent.right
          anchors.leftMargin: Theme.horizontalPageMargin
          anchors.rightMargin: Theme.horizontalPageMargin
          text: model.name
        }

        onClicked: {
          dialog.routeId = model.id;
          dialog.accept();
        }
      }

      ViewPlaceholder {
        enabled: routes.model.count === 0
        text: "No routes."
        hintText: "Tap route edit button in Chart view to create a route."
      }

      Component.onCompleted: {
        if (model.count > 0) {
          dialog.routeId = model.get("id", 0);
        }
      }
    }
    VerticalScrollDecorator {flickable: flickable}
  }
}
