import QtQuick 2.0
import Sailfish.Silica 1.0

Dialog {
  id: dialog

  property var tracker: undefined
  property bool saveNeeded: true

  onAccepted: {
    if (saveNeeded) {
      console.log("saving track");
      tracker.save();
    }
  }

  SilicaFlickable {
    id: flickable
    anchors.fill: parent

    Column {
      anchors.fill: parent

      DialogHeader {
        title: "Stop tracking"
        acceptText: "Save"
        cancelText: "Cancel"
      }

      IconListItem {
        label: "Pause"
        icon: "image://theme/icon-m-pause"
        onClicked: {
          dialog.tracker.pause();
          dialog.saveNeeded = false;
          dialog.accept();
        }
      }

      IconListItem {
        label: "Delete"
        icon: "image://theme/icon-m-clear"
        onClicked: {
          dialog.tracker.remove();
          dialog.saveNeeded = false;
          dialog.accept();
        }
      }
    }

    VerticalScrollDecorator {flickable: flickable}
  }
}
