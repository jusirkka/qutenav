import QtQuick 2.2
import Sailfish.Silica 1.0
import org.qopencpn 1.0

Page {

  id: page

  allowedOrientations: Orientation.Landscape | Orientation.Portrait

  property bool infoMode
  property bool posValid
  property bool headingValid
  property bool boatCentered: centerButton.centered
  property var mouseMoveHandler
  property var position: gps.position
  property var lastPos

  function panModeMouseMoveHandler(m) {
    if (centerButton.centered) centerButton.centered = false;
    encdis.pan(m.x, m.y)
    if (boat.visible && lastPos !== null) {
      boat.center = encdis.position(lastPos.coordinate.longitude, lastPos.coordinate.latitude)
    }
  }

  function infoModeMouseMoveHandler(m) {
    infoPoint.peepHole = Qt.point(infoPoint.peepHole.x + m.x - mouse.prev.x,
                                  infoPoint.peepHole.y + m.y - mouse.prev.y);
    mouse.prev = Qt.point(m.x, m.y);
  }

  Component.onCompleted: {
    app.encdis = encdis
    infoMode = false;
    posValid = false;
    headingValid = false;
    page.mouseMoveHandler = page.panModeMouseMoveHandler
    lastPos = null
  }

  onPositionChanged: {
    if (position.horizontalAccuracyValid) {
      posValid = position.horizontalAccuracy < 100;
    } else {
      // A hack for simulation mode. Remove after testing
      posValid = true;
    }
    if (posValid && position.longitudeValid && position.latitudeValid) {
      lastPos = position;
      if (centerButton.centered) {
        encdis.setEye(position.coordinate.longitude, position.coordinate.latitude);
      } else {
        boat.center = encdis.position(position.coordinate.longitude, position.coordinate.latitude);
      }
    }
    if (position.speedValid && position.directionValid) {
      headingValid = position.speed > .5;
      if (headingValid) {
        var p = encdis.advance(position.coordinate.longitude, position.coordinate.latitude,
                               3600 * position.speed, position.direction);
        boat.heading = Qt.point(p.x - boat.center.x, p.y - boat.center.y);
      }
    }
  }

  onBoatCenteredChanged: {
    if (boatCentered) {
      boat.center = Qt.point(width / 2, height / 2);
      if (lastPos !== null) {
        encdis.setEye(lastPos.coordinate.longitude, lastPos.coordinate.latitude)
      }
    }
  }

  ChartDisplay {
    id: encdis
    z: 150
    anchors.fill: parent
    onInfoQueryReady: {
      app.showInfo(info);
    }
  }

  Boat {
    id: boat
    z: 300
    visible: !page.infoMode && page.posValid && !zoom.zooming
    hasHeading: headingValid
  }

  MenuButton {
    id: menuButton
    z: 300
    visible: !page.infoMode
  }

  CenterButton {
    id: centerButton
    z: 300
    visible: !page.infoMode
  }

  ScaleBar {
    id: scaleBar
    z: 300
    visible: !page.infoMode
    scaleWidth: encdis.scaleBarLength
    onScaleWidthChanged: scaleBar.text = encdis.scaleBarText
  }

  CrossHairs {
    id: infoPoint
    z: 300
    visible: page.infoMode
  }

  Timer {
    id: infoTimer
    interval: 1500
    repeat: false
    onTriggered: {
      encdis.infoQuery(infoPoint.peepHole);
    }
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
        infoTimer.stop();
      }
      onReleased: {
        var p = Qt.point(mouse.x, mouse.y);
        if (p !== first) {
          if (page.infoMode) {
            infoTimer.start();
          }
          return;
        }
        page.infoMode = !page.infoMode;
        if (page.infoMode) {
          page.mouseMoveHandler = page.infoModeMouseMoveHandler;
          infoPoint.peepHole = Qt.point(page.width / 2, page.height / 2);
        } else {
          page.mouseMoveHandler = page.panModeMouseMoveHandler;
          infoTimer.stop();
        }
      }
    }
  }
}



