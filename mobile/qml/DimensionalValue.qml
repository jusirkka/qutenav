import QtQuick 2.0

Item {

  id: item

  property real value
  property int fontSize
  property string unit

  implicitHeight: left.implicitHeight
  implicitWidth: left.implicitWidth + rbot.implicitWidth

  Text {
    id: left
    text: "" + (!isNaN(item.value) ? Math.floor(item.value) : "-")
    font.pixelSize: item.fontSize
    font.family: "Arial Black"
  }

  Text {
    id: rbot
    anchors.left: left.right
    anchors.baseline: left.baseline
    text: "." + (!isNaN(item.value) ?
                   Math.floor(10 * (item.value - Math.floor(item.value))) : "-")
    font.pixelSize: item.fontSize / 2
    font.family: "Arial Black"
  }

  Text {
    id: rtop
    anchors.left: left.right
    anchors.baseline: rbot.top
    text: item.unit
    font.pixelSize: item.fontSize / 3
    font.family: "Arial Black"
  }
}
