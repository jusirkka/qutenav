import QtQuick 2.2

TrackInfoBox {
  id: info

  target: Item {

    width: arrow.width + target.width + padding
    height: target.height + 2 * padding

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

    Image {
      id: target

      anchors {
        left: arrow.right
        verticalCenter: parent.verticalCenter
        leftMargin: padding
        rightMargin: padding
      }

      height: 3.75 * padding
      width: height
      source: "icons/finish.png"
    }
  }
}
