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

  MouseArea {
    z: 100
    anchors.fill: parent

    onPressed: encdis.panStart(mouse.x, mouse.y)
    onPositionChanged: encdis.pan(mouse.x, mouse.y)
  }


}



