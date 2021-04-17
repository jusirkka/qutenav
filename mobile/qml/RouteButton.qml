import QtQuick 2.2
import Sailfish.Silica 1.0

Rectangle {
  id: rect

  property bool editing

  height: button.height * 1.45
  width: height
  radius: height / 2
  color: "white"
  border.color: "black"

  anchors.bottom: parent.bottom
  anchors.bottomMargin: Theme.paddingMedium
  anchors.rightMargin: Theme.paddingMedium

  onEditingChanged: {
    if (editing) {
      button.icon.color = "#214cad";
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
    icon.source: app.getIcon("route");
    icon.color: "black"

    onClicked: {
      if (rect.editing) {
        var dialog = pageStack.push(Qt.resolvedUrl("RoutingResultDialog.qml"), {router: router});
        dialog.onAccepted.connect(function () {
          console.log("stop editing");
          rect.editing = false;
        });
      } else {
        console.log("keep editing");
        rect.editing = true;
      }
    }
  }
}
