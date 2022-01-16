/* -*- coding: utf-8-unix -*-
 *
 * File: TrackDisplayDialog.qml
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
import Sailfish.Silica 1.0

import "./platform"

DialogPL {
  id: dialog

  //% "Select tracks to show"
  title: qsTrId("qutenav-track-display-title")
  //% "Show"
  acceptText: qsTrId("qutenav-track-display-accept-text")
  pageHeight: parent.height * .95
  hasOK: true

  ListViewPL {
    id: tracks

    anchors.fill: parent

    spacing: theme.paddingLarge

    model: TrackModel {
      id: trackModel
    }

    delegate: ListItemPL {
      contentHeight: s1.height
      menu: ContextMenuPL {
        ContextMenuItemPL {
          visible: model.preference < trackModel.topPreference()
          //% "Move to top"
          text: qsTrId("qutenav-track-display-menu-top")
          onClicked: {
            model.preference = trackModel.topPreference() + 1
            trackModel.sort()
          }
        }
        ContextMenuItemPL {
          //% "Rename"
          text: qsTrId("qutenav-track-display-menu-rename")
          onClicked: {
            var renamer = app.showAsDialog(Qt.resolvedUrl("Renamer.qml"), {name: model.name})
            renamer.onAccepted.connect(function () {
              model.name = renamer.name
            });
          }
        }
        ContextMenuItemPL {
          //% "Delete"
          text: qsTrId("qutenav-track-display-menu-delete")
          onClicked: {
            trackModel.remove(model.id);
          }
        }
      }

      Row {
        SwitchPL {
          id: s0
          visible: true
          Component.onCompleted: {
            checked = model.enabled
          }
          onCheckedChanged: {
            model.enabled = checked
          }
        }
        Column {
          id: s1
          width: dialog.width * 0.8
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
              text: units.displayDistance(trackModel.distance(model.id))
              anchors.right: parent.right
            }
          }
          Item {
            height: x21.height
            width: parent.width
            LabelPL {
              id: x21
              text: (new Date(trackModel.instant(model.id, 0))).toLocaleString(Qt.locale(), Locale.ShortFormat)
              font.pixelSize: theme.fontSizeSmall
              anchors.left: parent.left
            }
            LinkAreaPL {
              id: x22
              text: units.location(trackModel.location(model.id, 0), 0)
              anchors.right: parent.right
              onLinkActivated: {
                //% "Centering chart"
                info.notify(qsTrId("qutenav-centering-chart"))
                app.setEye(trackModel.location(model.id, 0))
              }
            }
          }
        }
      }
      onClicked: {
        console.log("show statistics")
        app.showAsDialog(Qt.resolvedUrl("TrackStatisticsPage.qml"), {
                           key: model.id,
                           name: model.name
                         })
      }
    }

    Bubble {
      id: info
    }

    ViewPlaceholderPL {
      enabled: tracks.model.count === 0
      //% "No tracks."
      text: qsTrId("qutenav-track-display-ph-text")
      //% "Tap tracks record button in Chart view to record a track."
      hintText: qsTrId("qutenav-track-display-ph-hint")
    }
  }
}


