import QtQuick 2.2
import Sailfish.Silica 1.0

Rectangle {
  id: rect


  height: button.height * 1.45
  width: height
  radius: 180
  color: "white"
  border.color: "black"
  opacity: 1.0

  anchors.bottom: parent.bottom
  anchors.bottomMargin: Theme.paddingMedium
  anchors.leftMargin: Theme.paddingMedium

  IconButton {
    id: button
    anchors.verticalCenter: parent.verticalCenter
    anchors.horizontalCenter: parent.horizontalCenter

    height: icon.sourceSize.height
    icon.smooth: false
    icon.source: app.getIcon("compass");
  }
}
