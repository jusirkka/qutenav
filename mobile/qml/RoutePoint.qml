import QtQuick 2.2
import Sailfish.Silica 1.0

Rectangle {
  id: rect

  z: 250

  property int index
  property point center
  property bool selected: false

  x: center.x - width / 2
  y: center.y - height / 2

  signal clicked(var rp)
  signal positionChanged(point delta)

  onSelectedChanged: {
    if (selected) {
      width = 4 * Theme.paddingLarge
      color = "cyan";
    } else {
      width = 3 * Theme.paddingLarge
      color = "white";
    }
    clicked(rect)
  }

  width: 3 * Theme.paddingLarge
  height: width
  radius: width / 2
  color: "white"
  border.color: "black"


  Text {
    id: label
    anchors.centerIn: parent
    text: "" + index
    color: "black"
    horizontalAlignment: Text.AlignHCenter
    verticalAlignment: Text.AlignVCenter
    font.pixelSize: Theme.fontSizeSmall
    font.bold: true
  }

  MouseArea {
    id: mouse
    anchors.fill: parent
    onClicked: {
      rect.selected = !rect.selected;
    }
  }

}
