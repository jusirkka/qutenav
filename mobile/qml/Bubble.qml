import QtQuick 2.0
import Sailfish.Silica 1.0

Rectangle {

  id: item

  anchors.top: parent.top
  anchors.topMargin: Theme.paddingLarge
  anchors.horizontalCenter: parent.horizontalCenter

  width: info.width + 2 * Theme.paddingLarge
  height: info.height + 2 * Theme.paddingLarge
  radius: 20
  color: "black"

  property string message
  property int tics: 16

  onVisibleChanged: {
    if (visible) {
      timer.counter = 0;
    }
  }

  Component.onCompleted: {
    visible = false;
  }

  Label {
    id: info
    text: item.message
    color: "cyan"
    anchors.horizontalCenter: parent.horizontalCenter
    anchors.verticalCenter: parent.verticalCenter
    font.pixelSize: Theme.fontSizeMedium

    ProgressBar {
      id: pbar
      height: 6
      anchors.left: parent.left
      anchors.right: parent.right
      anchors.bottom: parent.bottom
      anchors.bottomMargin: Theme.paddingSmall
      minimumValue: 0
      maximumValue: item.tics
      value: item.tics
    }

  }

  Timer {
    id: timer
    property int counter
    interval: 250
    repeat: true
    running: counter < item.tics
    onTriggered: {
      counter += 1;
      pbar.value = item.tics - counter;
    }

    onRunningChanged: {
      if (!running) {
        item.visible = false;
      }
    }

    Component.onCompleted: {
      counter = 10000;
    }

  }
}
