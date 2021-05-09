/* -*- coding: utf-8-unix -*-
 *
 * File: ApplicationWindowPL.qml
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
import QtQuick.Controls 2.12

ApplicationWindow {
  id: app

  visible: true

  property var dialog: null
  property var systemIcons: null

  default property alias initialPage: app.data

  Component.onCompleted: {
    width = settings.mainWindowWidth
    height = settings.mainWindowHeight
    systemIcons = {
      'clear': 'edit-clear',
      'save': 'document-save',
      'new': 'document-new',
      'edit': 'document-open',
      'pause': 'media-playback-pause',
      'archive': 'folder',
      'preferences': 'preferences-system',
    }
  }

  Component.onDestruction: {
    settings.mainWindowWidth = width
    settings.mainWindowHeight = height
  }

  function show(url, params) {
    if (dialog !== null) {
      dialog.close();
    }
    var component = Qt.createComponent(url);
    dialog = component.createObject(app, params ? params : {});
    return dialog;
  }

  function setPixelRatio() {
    // Return path to icon suitable for user's screen,
    // finding the closest match to Theme.pixelRatio.
    var ratios = [1.00, 1.25, 1.50, 1.75, 2.00]
    var minIndex = -1, minDiff = 1000, diff
    for (var i = 0; i < ratios.length; i++) {
      diff = Math.abs(Theme.pixelRatio - ratios[i]);
      minIndex = diff < minDiff ? i : minIndex;
      minDiff = Math.min(minDiff, diff);
    }
    app.pixelRatio = Math.floor(100 * ratios[minIndex])
    console.log("pixelratio", app.pixelRatio)
  }

  Shortcut {
    sequence: StandardKey.Quit
    context: Qt.ApplicationShortcut
    onActivated: Qt.quit()
  }

  Shortcut {
    sequence: StandardKey.ZoomIn
    context: Qt.ApplicationShortcut
    onActivated: encdis.zoomIn();
  }

  Shortcut {
    sequence: StandardKey.ZoomOut
    context: Qt.ApplicationShortcut
    onActivated: encdis.zoomOut();
  }

  Shortcut {
    sequence: "N"
    context: Qt.ApplicationShortcut
    onActivated: encdis.northUp();
  }

}


