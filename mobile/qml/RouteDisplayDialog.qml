import QtQuick 2.0
import Sailfish.Silica 1.0
import org.qopencpn 1.0

Dialog {
  id: dialog

  property var router: undefined
  property int routeId: -1

  onAccepted: {
    if (routeId > 0) {
      router.load(routeId);
    }
  }

  SilicaFlickable {
    id: flickable
    anchors.fill: parent

    DialogHeader {
      id: header
      title: "Select route"
      acceptText: "Most Recent"
      cancelText: "Cancel"
    }

    SilicaListView {
      id: routes

      anchors {
        top: header.bottom
        bottom: parent.bottom
        left: parent.left
        right: parent.right
        topMargin: Theme.paddingLarge
      }

      model: RouteModel {}

      delegate: ListItem {
        contentHeight: label.height + 2 * Theme.paddingMedium

        Label {
          id: label
          anchors.left: parent.left
          anchors.right: parent.right
          anchors.leftMargin: Theme.horizontalPageMargin
          anchors.rightMargin: Theme.horizontalPageMargin
          text: model.name
        }

        onClicked: {
          dialog.routeId = model.id;
          dialog.accept();
        }
      }

      ViewPlaceholder {
        enabled: routes.model.count === 0
        text: "No routes."
        hintText: "Tap route edit button in Chart view to create a route."
      }

      Component.onCompleted: {
        if (model.count > 0) {
          dialog.routeId = model.get("id", 0);
        }
      }
    }
    VerticalScrollDecorator {flickable: flickable}
  }
}
