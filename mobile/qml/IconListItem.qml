import QtQuick 2.2
import Sailfish.Silica 1.0

ListItem {
  id: item
  anchors.left: parent.left
  anchors.right: parent.right
  contentHeight: Theme.itemSizeSmall

  property string label: ""
  property string icon: ""
  property alias iconHeight: icon.sourceSize.height

  Image {
    id: icon
    fillMode: Image.PreserveAspectFit
    anchors.left: parent.left
    anchors.leftMargin: Theme.horizontalPageMargin
    anchors.verticalCenter: parent.verticalCenter
    sourceSize.height: Theme.itemSizeSmall * 0.8
    source: item.icon
  }

  Label {
    id: label
    anchors.left: icon.right
    anchors.leftMargin: Theme.paddingMedium
    anchors.right: parent.right
    anchors.rightMargin:Theme.horizontalPageMargin
    height: Theme.itemSizeSmall
    text: item.label
    verticalAlignment: Text.AlignVCenter
  }

}
