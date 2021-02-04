import QtQuick 2.2
import Sailfish.Silica 1.0
import org.qopencpn 1.0
import "."

Page {

  allowedOrientations: Orientation.Landscape | Orientation.Portrait

  Component.onCompleted: {
    app.encdis = encdis
  }

  ChartDisplay {
    id: encdis
    z: 150
    anchors.fill: parent
  }

  MenuButton {
    id: menuButton
    z: 300
  }

  ScaleBar {
    id: scaleBar
    z: 300
    scaleWidth: encdis.scaleBarLength
    onScaleWidthChanged: scaleBar.text = encdis.scaleBarText
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
      z: 100
      anchors.fill: parent

      onPressed: encdis.panStart(mouse.x, mouse.y)
      onPositionChanged: encdis.pan(mouse.x, mouse.y)
      onDoubleClicked: encdis.northUp()

    }
  }
}



