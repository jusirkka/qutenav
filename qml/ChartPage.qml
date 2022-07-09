/* -*- coding: utf-8-unix -*-
 *
 * File: mobile/qml/ChartPage.qml
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

import "./utils.js" as Util

import "./platform"

ChartPagePL {

  id: page

  property bool infoMode
  property bool posValid
  property bool headingValid
  property bool boatCentered: centerButton.centered
  property bool infoQueryPending
  property var mouseMoveHandler
  property var position: gps.position
  property var lastPos: undefined
  property var routePoint: undefined

  function panModeMouseMoveHandler(m) {
    if (routeButton.editing && routePoint !== undefined) {
      router.move(routePoint.index, Qt.point(m.x - mouse.prev.x, m.y - mouse.prev.y))
      mouse.prev = Qt.point(m.x, m.y)
      return
    }
    if (distButton.measuring && ruler.selection !== 0) {
      ruler.delta = Qt.point(m.x - mouse.prev.x, m.y - mouse.prev.y)
      mouse.prev = Qt.point(m.x, m.y)
      return
    }

    if (centerButton.centered) centerButton.centered = false

    encdis.pan(m.x, m.y)
    syncLayers()
  }

  function infoModeMouseMoveHandler(m) {
    infoPoint.peepHole = Qt.point(infoPoint.peepHole.x + m.x - mouse.prev.x,
                                  infoPoint.peepHole.y + m.y - mouse.prev.y)
    mouse.prev = Qt.point(m.x, m.y);
    if (!infoQueryPending) {
      infoQueryPending = true
      encdis.infoQuery(infoPoint.peepHole)
    }
  }

  function syncLayers(swap) {
    if (lastPos !== undefined) {
      if (!centerButton.centered) {
        boat.center = encdis.position(lastPos.coordinate)
      } else {
        if (swap !== undefined) {
          boat.center = Qt.point(height / 2, width / 2)
        } else {
          boat.center = Qt.point(width / 2, height / 2)
        }
        encdis.setEye(lastPos.coordinate)
      }
    }
    tracker.sync()
    router.sync()
    ci.sync()
    if (distButton.measuring) {
      ruler.sync()
    }
  }

  Component.onCompleted: {
    app.encdis = encdis
    app.syncLayers = syncLayers
    infoMode = false;
    posValid = false;
    headingValid = false;
    infoQueryPending = false;
    mouseMoveHandler = panModeMouseMoveHandler;
  }

  onOrientationChanged: {
    if (boat.visible && lastPos !== undefined && centerButton.centered) {
      // console.log(width, height)
      // WTF: width & height seem to have their old values at this point
      boat.center = Qt.point(height / 2, width / 2);
      encdis.setEye(lastPos.coordinate)
    }
    syncLayers(true);
    headingValid = false;
  }

  onHeightChanged: {
    syncLayers();
  }

  onWidthChanged: {
    syncLayers();
  }

  onPositionChanged: {
    if (position.horizontalAccuracyValid) {
      posValid = position.horizontalAccuracy < 100;
    } else {
      // A hack for simulation mode. Remove in production code
      posValid = true;
    }
    if (posValid && position.longitudeValid && position.latitudeValid) {
      lastPos = position;
      if (centerButton.centered) {
        encdis.setEye(position.coordinate);
      } else {
        boat.center = encdis.position(position.coordinate);
      }
      if (tracker.status === Tracker.Tracking) {
        tracker.append(position.coordinate);
      }
    }
    if (position.speedValid && position.directionValid) {
      headingValid = position.speed > .5;
      if (headingValid) {
        var p = encdis.advance(position.coordinate, 3600 * position.speed, position.direction);
        boat.heading = Qt.point(p.x - boat.center.x, p.y - boat.center.y);
      }
    }
  }

  onBoatCenteredChanged: {
    if (centerButton.centered && lastPos !== undefined) {
      boat.center = Qt.point(width / 2, height / 2);
      encdis.setEye(lastPos)
      syncLayers();
    }
  }

  function selectRoutePoint(rp) {
    if (routePoint !== undefined) {
      routePoint.selected = false;
    }
    if (rp.selected) {
      routePoint = rp;
    } else {
      routePoint = undefined;
    }
  }

  function loadRoute() {
    for (var i = 0; i < router.length(); i++) {
      var component = Qt.createComponent("RoutePoint.qml")
      var obj = component.createObject(router, {index: i, center: router.vertex(i)});
      obj.clicked.connect(selectRoutePoint);
    }
    router.enableEditMode(routeButton.editing)
  }

  ChartDisplay {
    id: encdis
    z: 150
    anchors.fill: parent

    onInfoQueryReady: {
      page.infoQueryPending = false;
      bubble.show(info, "image://s57/" + objectId,
                  infoPoint.peepHole.y > page.height / 2 ? "top" : "bottom")
    }

    onChartDBStatus: {
      bubble.show(msg);
    }

    Tracker {
      id: tracker
      z: 200
      visible: !page.infoMode &&
               (tracker.status === Tracker.Tracking || tracker.status === Tracker.Displaying) &&
               !zoom.zooming
    }

    Router {
      id: router
      z: 200
      visible: !page.infoMode && !empty && !zoom.zooming
      onDistanceChanged: {
        bubble.show(units.displayDistance(distance, 3))
      }
    }

    ChartIndicator {
      id: ci
      z: 200
      visible: !page.infoMode && !zoom.zooming
    }
  }

  Boat {
    id: boat
    z: 200
    visible: !page.infoMode && page.posValid && !zoom.zooming
    hasHeading: headingValid
  }

  BusyIndicatorPL {
    running: encdis.processingCharts
    anchors.centerIn: parent
    z: 400
  }

  TrackInfo {
    id: trackInfo
    visible: !page.infoMode && trackButton.tracking
    z: 300
  }

  MenuButton {
    id: menuButton
    anchors.horizontalCenter: parent.horizontalCenter
    z: 300
    visible: !page.infoMode
    routeLoader: page.loadRoute;
  }

  TrackButton {
    id: trackButton
    anchors.left: menuButton.right
    z: 300
    visible: !page.infoMode
  }

  RouteButton {
    id: routeButton
    anchors.right: menuButton.left
    z: 300
    visible: !page.infoMode
    onEditingChanged: {
      router.enableEditMode(editing)
    }
  }

  CenterButton {
    id: centerButton
    anchors.right: parent.right
    z: 300
    visible: !page.infoMode
    onCenteredChanged: {
      if (centered) {
        bubble.show(Util.printPos(lastPos));
      }
    }
  }

  DistanceButton {
    id: distButton
    anchors.left: parent.left
    z: 300
    visible: !page.infoMode
  }

  Ruler {
    id: ruler
    visible: distButton.measuring && !page.infoMode && !zoom.zooming
    measuring: distButton.measuring
    z: 300
  }

  MapLabel {
    id: bearingLabel
    visible: ruler.visible
    z: 300
    anchors.left: parent.left
    anchors.bottom: distButton.top
    label: encdis.displayBearing(ruler.c1, ruler.c2, ruler.selection === 1)
  }

  MapLabel {
    id: locationLabel
    visible: ruler.visible
    z: 300
    anchors.left: parent.left
    anchors.bottom: bearingLabel.top
    label: units.location(ruler.selection === 1 ? ruler.c1 : ruler.c2, 2)
  }

  EditButton {
    id: deleteButton
    anchors.left: parent.left
    anchors.bottom: distButton.top
    z: 300
    visible: routeButton.editing && routePoint !== undefined
    //% "Remove selected"
    label:  qsTrId("qutenav-edit-remove-selected")
    onClicked: {
      router.remove(routePoint.index);
      routePoint = undefined;
    }
  }

  EditButton {
    anchors.verticalCenter: deleteButton.verticalCenter
    anchors.left: deleteButton.right
    z: 300
    visible: routeButton.editing && routePoint !== undefined &&
             routePoint.index !== router.length() - 1
    //% "Insert after"
    label:  qsTrId("qutenav-edit-insert-after")
    onClicked: {
      var index = routePoint.index + 1;
      var qi = router.insert(index);
      console.log("Insert Routepoint", index);

      var component = Qt.createComponent("RoutePoint.qml")
      var obj = component.createObject(router, {index: index, center: qi});
      obj.clicked.connect(selectRoutePoint);

      routePoint.selected = false;
      routePoint = undefined;
    }
  }

  ScaleBar {
    id: scaleBar
    anchors.bottom: centerButton.top
    z: 300
    visible: !page.infoMode
    scaleWidth: encdis.scaleBarLength
    distanceText: encdis.scaleBarTexts[0]
    scaleText: encdis.scaleBarTexts[1]
    scaleText2: encdis.scaleBarTexts[2]
  }

  Bubble {
    id: bubble
    bottomItem: menuButton
    topItem: trackInfo
    z: 301
  }

  CrossHairs {
    id: infoPoint
    z: 300
    visible: page.infoMode
  }

  PinchArea {
    id: zoom

    property bool zooming

    anchors.fill: parent
    pinch.minimumRotation: -180
    pinch.maximumRotation: 180
    pinch.minimumScale: .5
    pinch.maximumScale: 2

    Component.onCompleted: {
      zooming = false;
    }

    onPinchStarted: {
      zooming = true;
    }

    onPinchFinished: {
      zooming = false;
      syncLayers();
    }

    onPinchUpdated: {
      if (pinch.scale > 0) {
        // zoom in/out
        var delta;
        if (pinch.scale > 1) {
          delta = Math.floor(10 * pinch.scale) - Math.floor(10 * pinch.previousScale);
          if (delta > 0) {
            encdis.zoomIn();
          }
        } else {
          delta = Math.floor(10. / pinch.scale) - Math.floor(10. / pinch.previousScale);
          if (delta > 0) {
            encdis.zoomOut();
          }
        }
      }
    }

    MouseArea {
      id: mouse
      z: 100
      anchors.fill: parent
      property point first
      property point prev

      onDoubleClicked: encdis.northUp()
      onPositionChanged: page.mouseMoveHandler(mouse)
      onPressed: {
        first = Qt.point(mouse.x, mouse.y);
        prev = first;
        encdis.panStart(mouse.x, mouse.y);
      }
      onReleased: {
        var p = Qt.point(mouse.x, mouse.y);
        if (p != first) {
          if (routeButton.editing && routePoint !== undefined) {
            routePoint.selected = false;
            routePoint = undefined;
          }
          return;
        }
        if (routeButton.editing) {
          var q = Qt.point(mouse.x, mouse.y);
          var index = router.append(q);
          console.log("Append Routepoint", index);
          var component = Qt.createComponent("RoutePoint.qml")
          var obj = component.createObject(router, {index: index, center: q});
          obj.clicked.connect(selectRoutePoint);
          return;
        }
        page.infoMode = !page.infoMode;
        if (page.infoMode) {
          page.mouseMoveHandler = page.infoModeMouseMoveHandler;
          infoPoint.peepHole = Qt.point(page.width / 2, page.height / 2);
        } else {
          page.mouseMoveHandler = page.panModeMouseMoveHandler;
        }
      }
    }
  }
}



