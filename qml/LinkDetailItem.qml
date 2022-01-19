import QtQuick 2.6
import org.qutenav 1.0

import "./platform"

Item {

  id: main
  width: parent.width
  height: Math.max(key.height, val.height + theme.paddingSmall)

  property alias label: key.text
  property alias value: val.linkText

  signal valueLinkActivated()

  anchors.horizontalCenter: parent.horizontalCenter

  Text {
    id: key
    width: parent.width / 2 - theme.paddingSmall
    horizontalAlignment:  Text.AlignRight
    color: theme.secondaryHighlightColor
    font.pixelSize: theme.fontSizeMedium
    textFormat: Text.PlainText
    wrapMode: Text.Wrap
  }

  LinkAreaPL {
    id: val
    width: parent.width / 2 - theme.paddingSmall
    anchors {
      left: parent.horizontalCenter
      leftMargin: theme.paddingSmall
      right: parent.right
      rightMargin: theme.paddingSmall
    }
    onLinkActivated: main.valueLinkActivated()
  }
}
