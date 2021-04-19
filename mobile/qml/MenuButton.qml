import QtQuick 2.2
import Sailfish.Silica 1.0

Rectangle {

  id: rect
  height: button.height * 1.45
  width: height
  radius: 180
  color: "white"
  border.color: "black"

  anchors.bottom: parent.bottom
  anchors.bottomMargin: Theme.paddingMedium
  anchors.rightMargin: Theme.paddingMedium
  anchors.leftMargin: Theme.paddingMedium

  property var routeLoader: undefined

  IconButton {
    id: button
    anchors.verticalCenter: parent.verticalCenter
    anchors.horizontalCenter: parent.horizontalCenter

    height: icon.sourceSize.height
    icon.smooth: false
    icon.source: app.getIcon("menu")
    icon.color: "black"

    onClicked: pageStack.push(Qt.resolvedUrl("MenuPage.qml"),
                              {tracker: tracker, router: router, routeLoader: routeLoader});

  }
}
