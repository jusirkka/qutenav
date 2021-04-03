import QtQuick 2.2
import Sailfish.Silica 1.0

CoverBackground {

  Image {
    id: logo
    anchors.centerIn: parent
    source: "/usr/share/icons/hicolor/128x128/apps/harbour-qopencpn.png"
  }

  Label {
    anchors.horizontalCenter: parent.horizontalCenter
    anchors.top: logo.bottom
    anchors.topMargin: Theme.paddingLarge
    text: "QOpenCPN"
  }
}


