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
import QtQuick 2.6

import org.qutenav 1.0

import "./platform"

PagePL {
  id: page

  property var tracker
  property var router
  property var routeLoader

  title: "QuteNav"

  pageHeight: parent.height

  IconListItemPL {
    //% "Preferences"
    label: qsTrId("qutenav-page-preferences")
    iconName: app.getSystemIcon('preferences')
    onClicked: app.show(Qt.resolvedUrl("PreferencesPage.qml"))
  }

  IconListItemPL {
    //% "Tracks"
    label: qsTrId("qutenav-page-tracks") + (enabled ? "" : "*")
    iconName: app.getSystemIcon('archive')
    onClicked: {
      var d1 = app.show(Qt.resolvedUrl("TrackDisplayDialog.qml"))
      d1.onAccepted.connect(page.tracker.display);
    }
    enabled: page.tracker.status !== Tracker.Tracking &&
             page.tracker.status !== Tracker.Paused
  }

  IconListItemPL {
    //% "Routes"
    label: qsTrId("qutenav-page-routes") + (enabled ? "" : "*")
    iconName: app.getSystemIcon('archive')
    onClicked: {
      var d2 = app.show(Qt.resolvedUrl("RouteDisplayDialog.qml"),
                        {
                          router: page.router,
                          tracker: page.tracker,
                          routeId: -1
                        })
      d2.onAccepted.connect(page.routeLoader);
    }
    enabled: !page.router.edited
  }

  IconListItemPL {
    //% "Chart folders"
    label: qsTrId("qutenav-page-chart-folders")
    iconName: app.getSystemIcon('documents')
    onClicked: {
      app.show(Qt.resolvedUrl("ChartDialog.qml"), {
                 paths: settings.chartFolders
               })
    }
  }

  ComboBoxPL {
    id: setBox
    //% "Chartsets"
    label: qsTrId("qutenav-page-chart-chartsets")
    //% "Select the type of charts to display."
    description: qsTrId("qutenav-page-chart-chartsets-description")
    Component.onCompleted: {
      model = encdis.chartSets;
    }
    onCurrentIndexChanged: {
      encdis.chartSet = model[currentIndex];
    }
    onModelChanged: {
      currentIndex = 0;
      var active = encdis.chartSet;
      for (var i = 0; i < model.length; i++) {
        if (model[i] === active) {
          currentIndex = i;
          break;
        }
      }
    }
  }

  TextSwitchPL {
    id: outline
    //% "Outline charts"
    text: qsTrId("qtnav-outline-charts")
    //% "Outline available larger scale charts on top of current set of charts."
    description: qsTrId("qtnav-outline-charts-description")
    Component.onCompleted: {
      outline.checked = settings.indicateScales;
    }
    onCheckedChanged: {
      settings.indicateScales = checked;
    }
  }

}

