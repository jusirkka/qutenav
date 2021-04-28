/* -*- coding: utf-8-unix -*-
 *
 * File: mobile/qml/MenuPage.qml
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
import QtQuick 2.2
import Sailfish.Silica 1.0
import org.qutenav 1.0

Page {
  id: page

  property var tracker: undefined
  property var router: undefined
  property var routeLoader: undefined

  SilicaFlickable {
    id: flickable
    anchors.fill: parent

    Column {

      anchors.fill: parent

      PageHeader {
        id: header
        title: "QuteNav"
      }

      IconListItem {
        label: "Preferences"
        icon: "image://theme/icon-m-setting"
        onClicked: pageStack.replace(Qt.resolvedUrl("PreferencesPage.qml"))
      }

      IconListItem {
        label: "Tracks" + (enabled ? "" : "*")
        icon: "image://theme/icon-m-file-archive-folder"
        onClicked: {
          var dialog = pageStack.replace(Qt.resolvedUrl("TrackDisplayDialog.qml"))
          dialog.onAccepted.connect(page.tracker.display);
        }
        enabled: page.tracker.status !== Tracker.Tracking &&
                 page.tracker.status !== Tracker.Paused
      }

      IconListItem {
        label: "Routes" + (enabled ? "" : "*")
        icon: "image://theme/icon-m-file-archive-folder"
        onClicked: {
          var dialog = pageStack.replace(Qt.resolvedUrl("RouteDisplayDialog.qml"),
                                         {router: page.router})
          dialog.onAccepted.connect(routeLoader);
        }
        enabled: !page.router.edited
      }

      ChartSetCombo {
        label: "Chartset"
        icon: "image://theme/icon-m-levels"
      }
    }


    VerticalScrollDecorator {flickable: flickable}
  }

}
