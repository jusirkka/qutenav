import QtQuick 2.2
import Sailfish.Silica 1.0

Rectangle {
  id: rect

  property string label

  signal clicked

  width: label.width + 2 * Theme.paddingLarge
  height: label.height + 2 * Theme.paddingMedium
  radius: width / 10
  color: "#214cad"
  border.color: "black"

  anchors.bottomMargin: Theme.paddingMedium
  anchors.leftMargin: Theme.paddingMedium
  anchors.rightMargin: Theme.paddingMedium


  Text {
    id: label
    anchors.centerIn: parent
    text: rect.label
    color: "white"
    horizontalAlignment: Text.AlignHCenter
    verticalAlignment: Text.AlignVCenter
    font.pixelSize: Theme.fontSizeSmall
    font.bold: true
  }

  MouseArea {
    id: mouse
    anchors.fill: parent

    onClicked: {
      rect.clicked();
    }
  }

}
