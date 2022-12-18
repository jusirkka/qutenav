/* -*- coding: utf-8-unix -*-
 *
 * Copyright (C) 2022 Jukka Sirkka
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
import "./utils.js" as Util

PagePL {

  id: page

  property int key
  property string name
  property var trackModel: TrackModel {}

  //% "Statistics"
  title: name + " " + qsTrId("qtnav-stats")
  pageHeight: parent.height

  SimpleBubble {
    id: info
    parent: page.contentItem
  }

  SectionHeaderPL {
    //% "Start"
    text: qsTrId("qtnav-stats-start")
  }

  LinkDetailItem {
    //% "Location"
    label: qsTrId("qtnav-stats-location")
    value: units.location(trackModel.location(key, 0), 0)
    onValueLinkActivated: {
      //% "Centering chart"
      info.notify(qsTrId("qutenav-centering-chart"))
      app.setEye(trackModel.location(key, 0))
    }
  }

  DetailItemPL {
    //% "Date"
    label: qsTrId("qtnav-stats-date")
    value: (new Date(trackModel.instant(key, 0))).toLocaleString(Qt.locale(), Locale.ShortFormat)
  }

  SectionHeaderPL {
    //% "Finish"
    text: qsTrId("qtnav-stats-finish")
  }

  LinkDetailItem {
    //% "Location"
    label: qsTrId("qtnav-stats-location")
    value: units.location(trackModel.location(key, -1), 0)
    onValueLinkActivated: {
      //% "Centering chart"
      info.notify(qsTrId("qutenav-centering-chart"))
      app.setEye(trackModel.location(key, -1))
    }
  }

  DetailItemPL {
    //% "Date"
    label: qsTrId("qtnav-stats-date")
    value: (new Date(trackModel.instant(key, -1))).toLocaleString(Qt.locale(), Locale.ShortFormat)
  }

  SectionHeaderPL {
    //% "Other numbers"
    text: qsTrId("qtnav-stats-others")
  }

  DetailItemPL {
    //% "Covered distance"
    label: qsTrId("qtnav-stats-dist")
    value: units.displayDistance(trackModel.distance(key))
  }

  DetailItemPL {
    //% "Straight line distance"
    label: qsTrId("qtnav-stats-straight-dist")
    value: units.displayDistance(trackModel.straightLineDistance(key))
  }

  DetailItemPL {
    //% "Bearing"
    label: qsTrId("qtnav-stats-bearing")
    value: "" + Util.pad(Math.floor(trackModel.bearing(key) + .5), "000") + "°"
  }

  DetailItemPL {
    //% "Duration"
    label: qsTrId("qtnav-stats-duration")
    value: Util.printDuration(trackModel.duration(key))
  }

  DetailItemPL {
    //% "Paused"
    label: qsTrId("qtnav-stats-paused")
    value: Util.printDuration(trackModel.pausedDuration(key))
  }

  DetailItemPL {
    //% "Average speed"
    label: qsTrId("qtnav-stats-av-speed")
    value: units.displaySpeed(trackModel.speed(key))
  }

  DetailItemPL {
    //% "Average speed when moving"
    label: qsTrId("qtnav-stats-av-speed-moving")
    value: units.displaySpeed(trackModel.speedWhileMoving(key))
  }

  DetailItemPL {
    //% "Straight line speed"
    label: qsTrId("qtnav-stats-st-speed")
    value: units.displaySpeed(trackModel.straightLineSpeed(key))
  }

  DetailItemPL {
    //% "Maximum speed"
    label: qsTrId("qtnav-stats-max-speed")
    value: units.displaySpeed(trackModel.maxSpeed(key))
  }

}
