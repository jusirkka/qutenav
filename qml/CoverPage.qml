/* -*- coding: utf-8-unix -*-
 *
 * File: mobile/qml/CoverPage.qml
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

CoverBackground {

  Image {
    id: logo
    anchors.centerIn: parent
    source: "/usr/share/icons/hicolor/128x128/apps/harbour-qutenav.png"
  }

  Label {
    anchors.horizontalCenter: parent.horizontalCenter
    anchors.top: logo.bottom
    anchors.topMargin: Theme.paddingLarge
    text: "QuteNav"
  }
}


