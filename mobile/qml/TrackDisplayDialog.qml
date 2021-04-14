import QtQuick 2.0
import Sailfish.Silica 1.0
import org.qopencpn 1.0

Dialog {
  id: dialog


  SilicaFlickable {
    id: flickable
    anchors.fill: parent

    Column {
      anchors.fill: parent

      DialogHeader {
        title: "Select tracks to show"
        acceptText: "Show"
        cancelText: "Cancel"
      }

      Repeater {
        model: TrackModel {}
        TextSwitch {
          Component.onCompleted: {
            checked = model.enabled;
            text = model.name
          }
          onCheckedChanged: {
            model.enabled = checked;
          }
        }
      }
    }

    VerticalScrollDecorator {flickable: flickable}
  }
}
