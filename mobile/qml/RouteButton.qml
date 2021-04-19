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
        if (router.edited) {
          var dialog1 = pageStack.push(Qt.resolvedUrl("RoutingResultDialog.qml"), {router: router});
          dialog1.onAccepted.connect(function () {
            rect.editing = false;
          });
        } else {
          rect.editing = false;
        }
      } else {
        if (!router.empty) {
          var dialog2 = pageStack.push(Qt.resolvedUrl("RoutingStartDialog.qml"), {router: router});
          dialog2.onAccepted.connect(function () {
            rect.editing = true;
          });
        } else {
          rect.editing = true;
        }
      }
    }
  }
}
