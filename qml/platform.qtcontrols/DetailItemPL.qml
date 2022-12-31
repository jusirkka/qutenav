import QtQuick 2.15
import QtQuick.Controls 2.12

Item {

  id: main
  width: parent.width

  implicitHeight: val.implicitHeight

  property alias label: key.text
  property alias value: val.text
  property real relativeKeyWidth: .5

  signal valueLinkActivated(string link)

  anchors.horizontalCenter: parent.horizontalCenter

  Label {
    id: key
    width: relativeKeyWidth * parent.width - theme.paddingSmall
    horizontalAlignment:  Text.AlignRight
    elide: Text.ElideLeft
  }

  Label {
    id: val
    width: (1 - relativeKeyWidth) * parent.width - theme.paddingSmall
    anchors {
      right: parent.right
      // verticalCenter: key.verticalCenter
    }
    wrapMode: Text.Wrap
    onLinkActivated: main.valueLinkActivated(link)
  }
}
