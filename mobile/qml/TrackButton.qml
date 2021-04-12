import QtQuick 2.2
import Sailfish.Silica 1.0

Rectangle {
  id: rect

  property bool tracking

  height: button.height * 1.45
  width: height
  radius: 180
  color: "white"
  border.color: "black"

  anchors.bottom: parent.bottom
  anchors.bottomMargin: Theme.paddingLarge
  anchors.rightMargin: Theme.paddingLarge
  anchors.leftMargin: Theme.paddingLarge

  onTrackingChanged: {
    if (tracking) {
      button.icon.color = "#ff0000";
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
    icon.source: app.getIcon("record");
    icon.color: "black"

    onClicked: {
      if (rect.tracking) {
        var dialog = pageStack.push(Qt.resolvedUrl("TrackResultDialog.qml"), {tracker: tracker});
        dialog.onAccepted.connect(function () {
          console.log("stop tracking");
          rect.tracking = false;
        });
      } else {
        console.log("keep tracking");
        rect.tracking = true;
      }
      tracker.sync();
    }
  }
}
