import QtQuick 2.0
import Sailfish.Silica 1.0

Rectangle {

  id: item

  implicitHeight: content.implicitHeight + title.implicitHeight + 2 * Theme.paddingLarge
  implicitWidth: content.implicitWidth + 2 * Theme.paddingLarge

  color: "#01356b"

  property string unit: undefined
  property string title: undefined
  property real value: undefined


  Item {
    id: content

    anchors.horizontalCenter: parent.horizontalCenter
    anchors.verticalCenter: parent.verticalCenter

    implicitHeight: left.implicitHeight
    implicitWidth: left.implicitWidth + rbot.implicitWidth

    Text {
      id: left
      text: "" + Math.floor(item.value)
      color: "white"
      font.pixelSize: Theme.fontSizeHuge
      font.family: "Arial Black"
    }

    Text {
      id: rbot
      anchors.left: left.right
      anchors.baseline: left.baseline
      text: "." + Math.floor(10 * (item.value - Math.floor(item.value)))
      color: "white"
      font.pixelSize: Math.floor(.6 * Theme.fontSizeHuge)
      font.family: "Arial Black"
    }

    Text {
      id: rtop
      anchors.left: left.right
      anchors.baseline: rbot.top
      text: item.unit
      color: "white"
      font.pixelSize: Math.floor(.4 * Theme.fontSizeHuge)
      font.family: "Arial Black"
    }
  }

  Text {
    id: title
    anchors.horizontalCenter: content.horizontalCenter
    anchors.top: content.bottom
    text: item.title
    color: "white"
    font.pixelSize: Theme.fontSizeSmall
  }


}
