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

import ".."

ApplicationWindow {
  id: aw

  visible: true

  property var dialog: null
  property var systemIcons: null
  property bool fullScreen
  property size fallbackGeom

  ChartPage {
    id: chartPage
  }

  Component.onCompleted: {
    systemIcons = {
      'clear': 'edit-clear',
      'save': 'document-save',
      'new': 'document-new',
      'edit': 'document-open',
      'pause': 'media-playback-pause',
      'archive': 'folder',
      'preferences': 'preferences-system',
      'documents': 'folder-documents',
    }
    width = settings.windowGeom.width
    height = settings.windowGeom.height
    fullScreen = settings.fullScreen
    if (fullScreen) {
      fallbackGeom = settings.lastGeom
      width = fallbackGeom.width
      height = fallbackGeom.height
      showFullScreen()
    }
  }

  Component.onDestruction: {
    settings.windowGeom = Qt.size(width, height)
    settings.lastGeom = fallbackGeom
    settings.fullScreen = fullScreen
  }

  function show(url, params) {
    if (dialog !== null) {
      dialog.close();
    }
    var component = Qt.createComponent(url);
    if (component.status === Component.Error) {
      console.log("Error: " + component.errorString())
      return null
    }
    dialog = component.createObject(app, params ? params : {})
    return dialog;
  }

  function showAsDialog(url, params) {
    var component = Qt.createComponent(url);
    if (component.status === Component.Error) {
      console.log("Error: " + component.errorString())
      return null
    }
    return component.createObject(app, params ? params : {})
  }

  Shortcut {
    sequence: StandardKey.Quit
    context: Qt.ApplicationShortcut
    onActivated: Qt.quit()
  }

  Shortcut {
    sequences: ["Ctrl+=", "Ctrl++"] // Do not use StandardKey.ZoomIn: QTBUG-105193
    context: Qt.ApplicationShortcut
    onActivated: {
      encdis.zoomIn()
      chartPage.syncLayers()
    }
  }

  Shortcut {
    sequence: StandardKey.ZoomOut
    context: Qt.ApplicationShortcut
    onActivated: {
      encdis.zoomOut()
      chartPage.syncLayers()
    }
  }

  Shortcut {
    sequence: "N"
    context: Qt.ApplicationShortcut
    onActivated: {
      encdis.northUp()
      chartPage.syncLayers()
    }
  }

  Shortcut {
    sequence: StandardKey.FullScreen
    context: Qt.ApplicationShortcut
    onActivated: {
      fullScreen = !fullScreen
      if (fullScreen) {
        fallbackGeom = Qt.size(width, height)
        showFullScreen()
      } else {
        showNormal()
      }
      chartPage.syncLayers()
    }
  }

}


