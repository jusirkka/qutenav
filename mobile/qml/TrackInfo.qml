import QtQuick 2.0

Row {

  id: row

  anchors.top: parent.top
  anchors.left: parent.left
  anchors.right: parent.right

  property int duration: undefined
  property real speed: undefined
  property real distance: undefined

  TimeBanner {
    width: row.width / 3
    title: "Total time"
    minutes: row.duration
  }

  Banner {
    width: row.width / 3
    unit: "Kn"
    title: "Speed"
    value: row.speed
  }

  Banner {
    width: row.width / 3
    unit: "Nm"
    title: "Distance"
    value: row.distance
  }
}
