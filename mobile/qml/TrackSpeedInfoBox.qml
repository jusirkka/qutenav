import QtQuick 2.2

TrackInfoBox {

  id: info

  property real speed: NaN

  target: Rectangle {
    id: speedBox
    color: "white"
    radius: 4
    height: speed.height
    width: 1.5 * height + 2 * padding

    DimensionalValue {
      id: speed

      anchors {
        left: parent.left
        leftMargin: padding / 2
        rightMargin: padding / 2
        centerIn: parent
      }

      unit: "Kn"
      value: info.speed
      fontSize: info.fontSize
    }
  }
}
