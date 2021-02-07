import QtQuick 2.2
import Sailfish.Silica 1.0

Column {

  id: root
  height: childrenRect.height
  width: parent.width
  spacing: Theme.paddingMedium

  property alias description: desc.text
  property alias label: label.text
  property real value

  Label {
    id: label
    font.pixelSize: Theme.fontSizeMedium
    height: implicitHeight
    width: parent.width
    wrapMode: Text.WordWrap
    color: Theme.primaryColor
  }

  Item {
    width: parent.width * .75 - Theme.horizontalPageMargin
    height: m.height
    anchors.leftMargin: Theme.horizontalPageMargin
    TextField {
      id: val
      height: implicitHeight
      width: parent.width - m.width
      font.pixelSize: Theme.fontSizeMedium
      inputMethodHints: Qt.ImhFormattedNumbersOnly
      color: Theme.highlightColor
      horizontalAlignment: TextInput.AlignHCenter
      Component.onCompleted: {
        text = root.value.toFixed(1);
      }
      onFocusChanged: {
        if (!val.focus) {
          root.value = Number(val.text)
        }
      }
    }
    Label {
      id: m
      anchors.left: val.right
      text: "meters"
      width: .5 * parent.width
      height: implicitHeight
    }
  }

  Label {
    id: desc
    font.pixelSize: Theme.fontSizeSmall
    height: text ? implicitHeight : 0
    width: parent.width
    visible: text
    wrapMode: Text.WordWrap
    color: Theme.secondaryColor
  }

}
