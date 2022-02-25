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

  property var router
  property int routeId

  //% "Select route"
  title: qsTrId("qutenav-route-display-title")
  //% "Top"
  acceptText: qsTrId("qutenav-route-display-accept-text")
  pageHeight: parent.height * .75

  onAccepted: {
    if (routeId > 0) {
      router.load(routeId);
    }
  }

  ListViewPL {
    id: routes
    anchors.fill: parent
    spacing: theme.paddingLarge

    model: RouteModel {
      id: routeModel
    }

    delegate: ListItemPL {
      contentHeight: s1.height
      view: routes
      anchors.horizontalCenter: parent.horizontalCenter
      menu: ContextMenuPL {
        ContextMenuItemPL {
          visible: model.preference < routeModel.topPreference()
          //% "Move to top"
          text: qsTrId("qutenav-context-menu-top")
          onClicked: {
            model.preference = routeModel.topPreference() + 1
            routeModel.sort()
          }
        }
        ContextMenuItemPL {
          //% "Rename"
          text: qsTrId("qutenav-context-menu-rename")
          onClicked: {
            var renamer = app.showAsDialog(Qt.resolvedUrl("Renamer.qml"), {name: model.name})
            renamer.onAccepted.connect(function () {
              model.name = renamer.name
            });
          }
        }
        ContextMenuItemPL {
          //% "Delete"
          text: qsTrId("qutenav-context-menu-delete")
          onClicked: {
            routeModel.remove(model.id);
          }
        }
      }

      Column {
        id: s1
        width: dialog.width - 2 * theme.paddingLarge
        anchors.horizontalCenter: parent.horizontalCenter
        spacing: theme.paddingSmall
        Item {
          height: x11.height
          width: parent.width
          LabelPL {
            id: x11
            text: model.name
            font.bold: true
            anchors.left: parent.left
          }
          LabelPL {
            id: x12
            text: units.displayDistance(routeModel.distance(model.id))
            anchors.right: parent.right
          }
        }
        Item {
          height: x21.height
          width: parent.width
          LabelPL {
            id: x21
            //% "wps"
            text: "" + routeModel.wayPointCount(model.id) + " " + qsTrId("qutenav-wps")
            font.pixelSize: theme.fontSizeSmall
            anchors.left: parent.left
          }
          LinkAreaPL {
            id: x22
            linkText: units.location(routeModel.location(model.id, 0), 0)
            anchors.right: parent.right
            onLinkActivated: {
              //% "Centering chart"
              info.notify(qsTrId("qutenav-centering-chart"))
              app.setEye(routeModel.location(model.id, 0))
            }
          }
        }
      }

      onClicked: {
        dialog.routeId = model.id;
        dialog.accept();
      }
    }

    Bubble {
      id: info
    }

    ViewPlaceholderPL {
      enabled: routes.model.count === 0
      //% "No routes."
      text: qsTrId("qutenav-route-display-ph-text")
      //% "Click route edit button in Chart view to create a route."
      hintText: qsTrId("qutenav-route-display-ph-hint")
    }

    Component.onCompleted: {
      if (model.count > 0) {
        dialog.routeId = model.get("id", 0);
      }
    }
  }
}

