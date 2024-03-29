/* -*- coding: utf-8-unix -*-
 *
 * File: mobile/qml/TrackInfo.qml
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

Row {

  id: row

  spacing: theme.paddingSmall

  anchors.top: parent.top
  anchors.left: parent.left

  TrackSpeedInfoBox {
    mps: tracker.speed
    seconds: tracker.duration
    meters: tracker.distance
    bearing: tracker.bearing
  }

  TrackPointInfoBox {
    index: tracker.segmentEndPoint
    seconds: tracker.segmentETA
    meters: tracker.segmentDTG
    bearing: tracker.segmentBearing
    visible: !router.empty && !router.edited
  }

  TrackTargetInfoBox {
    seconds: tracker.targetETA
    meters: tracker.targetDTG
    visible: !router.empty && !router.edited
  }
}
