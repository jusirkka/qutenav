import QtQuick 2.0
import Sailfish.Silica 1.0

Dialog {
  id: dialog

  property var router: undefined
  property bool saveNeeded: true

  onAccepted: {
    if (saveNeeded) {
      router.save();
    }
  }

  SilicaFlickable {
    id: flickable
    anchors.fill: parent

    Column {
      anchors.fill: parent

      DialogHeader {
        title: router.name()
        acceptText: "Save & Show"
        cancelText: "Cancel"
      }

      IconListItem {
        label: "Save & Show"
        icon: "image://theme/icon-m-device-upload"
        onClicked: {
          dialog.accept();
        }
      }

      IconListItem {
        label: "Save & Close"
        icon: "image://theme/icon-m-device-upload"
        onClicked: {
          dialog.router.save();
          dialog.router.clear();
          dialog.saveNeeded = false;
          dialog.accept();
        }
      }


      IconListItem {
        label: "Close without saving"
        icon: "image://theme/icon-m-clear"
        onClicked: {
          dialog.router.clear();
          dialog.saveNeeded = false;
          dialog.accept();
        }
      }
    }

    VerticalScrollDecorator {flickable: flickable}
  }
}
