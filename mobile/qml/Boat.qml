import QtQuick 2.2

Item {

  id: item

  height: img.height
  width: img.width

  property point center
  property point heading
  property bool hasHeading

  x: center.x - img.width / 2
  y: center.y - img.height / 2

  Image {
    id: img
    smooth: false
    visible: true
    source: app.getIcon("ship")
  }

  Rectangle {
    id: head
    x: img.width / 2
    y: img.height / 2 - height / 2
    visible: item.hasHeading
    color: "red"
    height: 4
    width: Math.sqrt(item.heading.x * item.heading.x + item.heading.y * item.heading.y)
    transform: Rotation {
      origin.x: 0
      origin.y: head.height / 2
      angle: Math.atan2(item.heading.y, item.heading.x) * 180 / Math.PI
    }
  }

}
