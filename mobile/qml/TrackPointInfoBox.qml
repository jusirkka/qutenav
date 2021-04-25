import QtQuick 2.2

TrackInfoBox {
  id: info

  property int index: 0

  target: Item {

    width: arrow.width + wp.width + padding
    height: wp.height + 2 * padding

    Image {
      id: arrow

      anchors {
        left: parent.left
        verticalCenter: parent.verticalCenter
      }

      x: padding
      height: 2.5 * padding
      width: height
      source: "icons/arrow.png"
    }

    Rectangle {

      id: wp

      anchors {
        left: arrow.right
        verticalCenter: parent.verticalCenter
        leftMargin: padding
        rightMargin: padding
      }

      height: label.height + padding / 2
      width: height
      radius: width / 2
      color: "#085efa"
      border.color: "white"
      border.width: padding / 4

      Text {
        id: label
        anchors.centerIn: parent
        text: "" + info.index
        color: "white"
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        font.pixelSize: info.fontSize * .6
        font.bold: true
      }
    }
  }
}
