import QtQuick 2.0
import Sailfish.Silica 1.0

Rectangle {

  id: item

  implicitHeight: clock.implicitHeight + title.implicitHeight + 2 * Theme.paddingLarge
  implicitWidth: clock.implicitWidth + 2 * Theme.paddingLarge

  color: "#01356b"

  property int minutes: 0
  property string title: undefined

  function pad(s) {
    return String("00" + s).slice(- 2);
  }

  Text {
    id: clock
    anchors.horizontalCenter: parent.horizontalCenter
    anchors.verticalCenter: parent.verticalCenter
    text: "" + item.pad(Math.floor(item.minutes / 60)) + ":" + item.pad(item.minutes % 60)
    color: "white"
    font.pixelSize: Theme.fontSizeHuge
    font.family: "Arial Black"
  }

  Text {
    id: title
    anchors.horizontalCenter: parent.horizontalCenter
    anchors.top: clock.bottom
    text: item.title
    color: "white"
    font.pixelSize: Theme.fontSizeSmall
  }


}
