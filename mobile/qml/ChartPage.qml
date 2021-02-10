import QtQuick 2.2
import Sailfish.Silica 1.0
import org.qopencpn 1.0
import "."

Page {

  id: page

  allowedOrientations: Orientation.Landscape | Orientation.Portrait

  property bool infoMode
  property var mouseMoveHandler

  function panModeMouseMoveHandler(m) {
    encdis.pan(m.x, m.y)
  }

  function infoModeMouseMoveHandler(m) {
    infoPoint.peepHole = Qt.point(infoPoint.peepHole.x + m.x - mouse.prev.x,
                                  infoPoint.peepHole.y + m.y - mouse.prev.y);
    mouse.prev = Qt.point(m.x, m.y);
  }

  Component.onCompleted: {
    app.encdis = encdis
    page.infoMode = false;
    page.mouseMoveHandler = page.panModeMouseMoveHandler
  }

  ChartDisplay {
    id: encdis
    z: 150
    anchors.fill: parent
    onInfoQueryReady: {
      app.showInfo(info);
    }
  }

  MenuButton {
    id: menuButton
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
    anchors.fill: parent
    pinch.minimumRotation: -180
    pinch.maximumRotation: 180
    pinch.minimumScale: .5
    pinch.maximumScale: 2

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
        encdis.panStart(mouse.x, mouse.y)
        infoTimer.stop();
      }
      onReleased: {
        var p = Qt.point(mouse.x, mouse.y)
        if (p !== first) {
          if (page.infoMode) {
            infoTimer.start();
          }
          return;
        }
        page.infoMode = !page.infoMode;
        if (page.infoMode) {
          page.mouseMoveHandler = page.infoModeMouseMoveHandler
          infoPoint.peepHole = Qt.point(page.width / 2, page.height / 2);
        } else {
          page.mouseMoveHandler = page.panModeMouseMoveHandler
          infoTimer.stop();
        }
      }
    }
  }
}



