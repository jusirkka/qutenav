import QtQuick 2.2

Item {

  height: img.height
  width: img.width

  property point center

  x: center.x - img.width / 2
  y: center.y - img.height / 2

  Image {
    id: img
    smooth: false
    visible: true
    source: app.getIcon("ship")
  }
}
