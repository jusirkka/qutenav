import QtQuick 2.0
import Sailfish.Silica 1.0
import org.qopencpn 1.0

Dialog {
  id: dialog


  SilicaFlickable {
    id: flickable
    anchors.fill: parent

    DialogHeader {
      id: header
      title: "Select tracks to show"
      acceptText: "Show"
      cancelText: "Cancel"
    }

    SilicaListView {
      id: tracks

      anchors {
        top: header.bottom
        bottom: parent.bottom
        left: parent.left
        right: parent.right
        topMargin: Theme.paddingLarge
      }

      model: TrackModel {}

      delegate: TextSwitch {

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.leftMargin: Theme.horizontalPageMargin
        anchors.rightMargin: Theme.horizontalPageMargin

        Component.onCompleted: {
          checked = model.enabled;
          text = model.name
        }
        onCheckedChanged: {
          model.enabled = checked;
        }
      }

      ViewPlaceholder {
        enabled: tracks.model.count === 0
        text: "No tracks."
        hintText: "Tap tracks record button in Chart view to record a track."
      }
    }

    VerticalScrollDecorator {flickable: flickable}
  }
}
