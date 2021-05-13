import QtQuick 2.6
import Sailfish.Silica 1.0
import QtGraphicalEffects 1.0

Rectangle {
  id: rect

  property string iconSource
  property string iconColor

  height: image.height * 1.45
  width: height
  radius: height
  color: "white"
  border.color: "black"
  opacity: 1


  signal clicked

  Image {
    id: image
    anchors.centerIn: parent
    source: rect.iconSource
  }

  ColorOverlay {
    anchors.fill: image
    source: image
    color: iconColor
  }

  MouseArea {
    id: mouse
    anchors.fill: parent
    onClicked: rect.clicked()
  }
}

