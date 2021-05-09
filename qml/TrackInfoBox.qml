/* -*- coding: utf-8-unix -*-
 *
 * File: mobile/qml/TrackInfoBox.qml
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

Rectangle {
  id: rect

  property int fontSize: Theme.fontSizeMedium * 1.3
  property int padding: Theme.paddingSmall

  property real seconds
  property real dist
  property real bearing: NaN

  property alias target: upperLeft.sourceComponent

  height: upperBox.height + lowerBox.height + 3 * padding
  width: upperLeft.width + upperBox.width + 3 * padding
  radius: padding / 2
  color: "#01356b"

  function pad(s, p) {
    return String(p + s).slice(- p.length);
  }

  Loader {
    id: upperLeft

    anchors {
      left: parent.left
      top: parent.top
      leftMargin: padding
      rightMargin: padding
      topMargin: padding
    }
  }

  Rectangle {
    id: upperBox
    color: "white"
    radius: padding / 2
    height: lowerBox.height
    width: Math.max(clock.width + padding, 2.5 * clock.height)

    anchors {
      left: upperLeft.right
      top: parent.top
      leftMargin: padding
      rightMargin: padding
      topMargin: padding
    }

    Text {
      id: clock
      anchors {
        centerIn: parent
        left: parent.left
        leftMargin: padding / 2
      }
      text: "" + (!isNaN(rect.seconds) ?
                    rect.pad(Math.floor(rect.seconds / 3600), "00") + ":" +
                    rect.pad(Math.floor((rect.seconds % 3600) / 60), "00") : "--:--")
      font.pixelSize: rect.fontSize * .8
      font.family: "Arial Black"
      horizontalAlignment: Text.AlignHCenter
      verticalAlignment: Text.AlignVCenter
    }

  }

  Rectangle {
    id: lowerBox
    color: "white"
    radius: padding / 2
    height: dist.height
    width: dist.width + bearing.width + 3 * padding
    anchors {
      left: parent.left
      right: parent.right
      top: upperBox.bottom
      leftMargin: padding
      rightMargin: padding
      topMargin: padding
      bottomMargin: padding
    }

    DimensionalValue {

      id: dist

      anchors {
        left: parent.left
        leftMargin: padding / 2
        rightMargin: 2.5 * padding
      }
      unit: "Nm"
      value: rect.dist
      fontSize: rect.fontSize
    }

    Text {

      id: bearing

      anchors {
        left: dist.right
        verticalCenter: dist.verticalCenter
        leftMargin: 2.5 * padding
        rightMargin: 2 * padding
      }
      font.pixelSize: rect.fontSize * .8
      text: (!isNaN(rect.bearing)) ?
              ("" + pad(Math.floor(rect.bearing + .5), "000") + "°") : "---°"
      font.family: "Arial Black"
    }
  }

}
