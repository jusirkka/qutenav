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

import QtQuick 2.6
import Sailfish.Silica 1.0

ApplicationWindow {

  property var systemIcons: null

  cover: Component {CoverPage {}}
  initialPage: Component {ChartPage {}}

  Component.onCompleted: {
    systemIcons = {
      'clear': 'image://theme/icon-m-clear',
      'save': 'image://theme/icon-m-device-upload',
      'new': 'image://theme/icon-m-new',
      'edit': 'image://theme/icon-m-edit-selected',
      'pause': 'image://theme/icon-m-pause',
      'archive': 'image://theme/icon-m-file-archive-folder',
      'preferences': 'image://theme/icon-m-setting',
      'documents': 'image://theme/icon-m-document',
    }
  }


  function show(url, params) {
    if (pageStack.depth > 1) {
      return pageStack.replace(url, params ? params : {})
    }
    return pageStack.push(url, params ? params : {})
  }

  function showAsDialog(url, params) {
    return pageStack.push(url, params ? params : {})
  }
}

