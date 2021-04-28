import QtQuick 2.2
import Sailfish.Silica 1.0

Rectangle {
  id: rect

  z: 250

  property int index
  property point center
  property bool selected: false
  property bool editMode: true

  x: center.x - width / 2
  y: center.y - height / 2

  signal clicked(var rp)

  onSelectedChanged: {
    clicked(rect)
  }

  width: (rect.editMode ? (rect.selected ? 4 : 3) : 2)  * Theme.paddingLarge
  height: width
  radius: width / 2
  color: rect.editMode ? (rect.selected ? "cyan" : "white") : "#085efa"
  border.color: rect.editMode ? "black" : "white"

  Text {
    id: label
    anchors.centerIn: parent
    text: "" + index
    color: rect.editMode ? "black" : "white"
    horizontalAlignment: Text.AlignHCenter
    verticalAlignment: Text.AlignVCenter
    font.pixelSize: rect.editMode ? Theme.fontSizeSmall : Theme.fontSizeExtraSmall
    font.bold: true
  }

  MouseArea {
    id: mouse
    anchors.fill: parent
    onClicked: {
      rect.selected = !rect.selected;
    }
    enabled: rect.editMode
  }

}
