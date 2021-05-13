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

PagePL {
  id: page

  property var tracker: undefined
  property var router: undefined
  property var routeLoader: undefined

  title: "QuteNav"

  pageHeight: parent.height



  IconListItemPL {
    label: "Preferences"
    iconName: app.getSystemIcon('preferences')
    onClicked: app.show(Qt.resolvedUrl("PreferencesPage.qml"))
  }

  IconListItemPL {
    label: "Tracks" + (enabled ? "" : "*")
    iconName: app.getSystemIcon('archive')
    onClicked: {
      var dialog = app.show(Qt.resolvedUrl("TrackDisplayDialog.qml"))
      dialog.onAccepted.connect(page.tracker.display);
    }
    enabled: page.tracker.status !== Tracker.Tracking &&
             page.tracker.status !== Tracker.Paused
  }

  IconListItemPL {
    label: "Routes" + (enabled ? "" : "*")
    iconName: app.getSystemIcon('archive')
    onClicked: {
      var dialog = app.show(Qt.resolvedUrl("RouteDisplayDialog.qml"),
                            {router: page.router})
      dialog.onAccepted.connect(page.routeLoader);
    }
    enabled: !page.router.edited
  }

  IconListItemPL {
    label: "Chart folders"
    iconName: app.getSystemIcon('documents')
    onClicked: {
      var dialog = app.show(Qt.resolvedUrl("ChartDialog.qml"),
                            {paths: settings.chartFolders})
      dialog.onAccepted.connect(function () {
        settings.chartFolders = dialog.paths
        encdis.updateChartDB(dialog.fullUpdate)
      });
    }
  }

  ComboBoxPL {
    id: setBox
    label: "Chartsets"
    description: "Select the type of charts to display."
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
}

