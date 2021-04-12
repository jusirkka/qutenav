import QtQuick 2.2
import Sailfish.Silica 1.0

Page {
  id: page

  SilicaFlickable {
    id: flickable
    anchors.fill: parent

    Column {

      anchors.fill: parent

      PageHeader {
        id: header
        title: "QOpenCPN"
      }

      IconListItem {
        label: "Preferences"
        icon: "image://theme/icon-m-developer-mode"
        onClicked: pageStack.replace(Qt.resolvedUrl("PreferencesPage.qml"))
      }

      ChartSetCombo {
        label: "Chartset"
        icon: "image://theme/icon-m-levels"
      }

    }

    VerticalScrollDecorator {flickable: flickable}
  }

}
