import QtQuick 2.2
import Sailfish.Silica 1.0

Rectangle {
  id: rect

  property bool centered

  height: button.height * 1.45
  width: height
  radius: 180
  color: "white"
  border.color: "black"

  anchors.bottom: parent.bottom
  anchors.bottomMargin: Theme.paddingMedium
  anchors.rightMargin: Theme.paddingMedium

  onCenteredChanged: {
    if (centered) {
      button.icon.color = "#00b000";
    } else {
      button.icon.color = "black";
    }
  }

  IconButton {
    id: button
    anchors.verticalCenter: parent.verticalCenter
    anchors.horizontalCenter: parent.horizontalCenter

    height: icon.sourceSize.height
    icon.smooth: false
    icon.source: app.getIcon("center");
    icon.color: "black"

    onClicked: {
      rect.centered = true;
    }
  }
}
