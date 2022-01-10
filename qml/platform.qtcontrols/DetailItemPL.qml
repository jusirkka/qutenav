import QtQuick 2.15
import QtQuick.Controls 2.12

Item {

  id: main
  width: parent.width
  // height: label.height

  implicitHeight: key.implicitHeight

  property alias label: key.text
  property alias value: val.text

  signal linkActivated(string link)

  anchors.horizontalCenter: parent.horizontalCenter

  Label {
    id: key
    width: parent.width / 2 - theme.paddingSmall
    horizontalAlignment:  Text.AlignRight
    elide: Text.ElideLeft
  }

  Label {
    id: val
    width: parent.width / 2 - theme.paddingSmall
    anchors {
      right: parent.right
      verticalCenter: key.verticalCenter
    }
    onLinkActivated: main.linkActivated(link)
  }
}
