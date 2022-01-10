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
import QtQuick 2.6
import org.qutenav 1.0
import "./platform"

DialogPL {
  id: dialog

  property var router: undefined
  property int routeId: -1

  title: "Select route"
  acceptText: "Most Recent"

  pageHeight: parent.height * .75

  onAccepted: {
    if (routeId > 0) {
      router.load(routeId);
    }
  }

  ListViewPL {
    id: routes

    anchors.fill: parent

    model: RouteModel {}

    delegate: ListViewDelegatePL {

      LabelPL {
        id: label
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.leftMargin: theme.horizontalPageMargin
        anchors.rightMargin: theme.horizontalPageMargin
        text: model.name
      }

      onClicked: {
        dialog.routeId = model.id;
        dialog.accept();
      }
    }

    ViewPlaceholderPL {
      enabled: routes.model.count === 0
      text: "No routes."
      hintText: "Click route edit button in Chart view to create a route."
    }

    Component.onCompleted: {
      if (model.count > 0) {
        dialog.routeId = model.get("id", 0);
      }
    }
  }
}

