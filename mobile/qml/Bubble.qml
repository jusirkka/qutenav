import QtQuick 2.0
import Sailfish.Silica 1.0

Rectangle {

  id: item

  anchors.topMargin: Theme.paddingLarge
  anchors.bottomMargin: Theme.paddingLarge
  anchors.horizontalCenter: parent.horizontalCenter

  width: info.width + 2 * Theme.paddingLarge
  height: info.height + 2 * Theme.paddingLarge
  radius: 20
  color: "white"
  border.color: "black"

  visible: timer.running

  state: "top"

  states: [
    State {
      name: "bottom"
      AnchorChanges {
        anchors.bottom: parent.bottom
        target: item
      }
    },
    State {
      name: "top"
      AnchorChanges {
        anchors.top: parent.top
        target: item
      }
    }
  ]

  property int tics: 16

  function show(msg, pos) {
    info.text = msg;
    timer.start();
    state = pos;
  }

  Label {
    id: info
    color: "black"
    anchors.horizontalCenter: parent.horizontalCenter
    anchors.verticalCenter: parent.verticalCenter
    font.pixelSize: Theme.fontSizeMedium
  }

  Timer {
    id: timer
    interval: 4000
    repeat: false
    running: false
  }
}
