/* -*- coding: utf-8-unix -*-
 *
 * File: mobile/qml/qutenav.qml
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
import QtPositioning 5.2

import "./platform"

ApplicationWindowPL {

  id: app

  property var encdis: null
  property int pixelRatio: 100

  // for testing
  property int startInstant

  ThemePL {
    id: theme
  }

  PositionSource {
    id: gps
  }

  ChartPage {
    id: chartPage
  }

  Component.onCompleted: {
    setPixelRatio()
    // gps.nmeaSource = "file:///tmp/nmea.log"
    gps.start()
    startInstant = Date.now() / 1000;
  }

  Component.onDestruction: {
    encdis.advanceNMEALog(Date.now() / 1000 - startInstant);
  }

  function setPixelRatio() {
    // Return path to icon suitable for user's screen,
    // finding the closest match to theme.pixelRatio.
    var ratios = [1.00, 1.25, 1.50, 1.75, 2.00]
    var minIndex = -1, minDiff = 1000, diff
    for (var i = 0; i < ratios.length; i++) {
      diff = Math.abs(theme.pixelRatio - ratios[i]);
      minIndex = diff < minDiff ? i : minIndex;
      minDiff = Math.min(minDiff, diff);
    }
    app.pixelRatio = Math.floor(100 * ratios[minIndex])
  }

  function getIcon(name) {
    return Qt.resolvedUrl("icons/%1-%2.png".arg(name).arg(app.pixelRatio))
  }

  function getSystemIcon(name) {
    return systemIcons[name]
  }

  function setEye(coord) {
    encdis.setEye(coord)
    chartPage.syncLayers()
  }

}


