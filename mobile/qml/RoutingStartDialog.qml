import QtQuick 2.0
import Sailfish.Silica 1.0

Dialog {
  id: dialog

  property var router: undefined

  SilicaFlickable {
    id: flickable
    anchors.fill: parent

    Column {
      anchors.fill: parent

      DialogHeader {
        title: "Route editor"
        acceptText: "Edit current"
        cancelText: "Cancel"
      }

      IconListItem {
        label: "Edit current"
        icon: "image://theme/icon-m-edit-selected"
        onClicked: {
          dialog.accept();
        }
      }

      IconListItem {
        label: "Create new route"
        icon: "image://theme/icon-m-new"
        onClicked: {
          dialog.router.clear();
          dialog.accept();
        }
      }

      IconListItem {
        label: "Close current"
        icon: "image://theme/icon-m-clear"
        onClicked: {
          dialog.router.clear();
          dialog.reject();
        }
      }

    }

    VerticalScrollDecorator {flickable: flickable}
  }
}
