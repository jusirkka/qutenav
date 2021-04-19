import QtQuick 2.2
import Sailfish.Silica 1.0
import org.qopencpn 1.0

Page {
  id: page

  property var tracker: undefined
  property var router: undefined
  property var routeLoader: undefined

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
        icon: "image://theme/icon-m-setting"
        onClicked: pageStack.replace(Qt.resolvedUrl("PreferencesPage.qml"))
      }

      ChartSetCombo {
        label: "Chartset"
        icon: "image://theme/icon-m-levels"
      }

      IconListItem {
        label: "Tracks" + (enabled ? "" : "*")
        icon: "image://theme/icon-m-file-archive-folder"
        onClicked: {
          var dialog = pageStack.replace(Qt.resolvedUrl("TrackDisplayDialog.qml"))
          dialog.onAccepted.connect(page.tracker.display);
        }
        enabled: page.tracker.status !== Tracker.Tracking &&
                 page.tracker.status !== Tracker.Paused
      }

      IconListItem {
        label: "Routes" + (enabled ? "" : "*")
        icon: "image://theme/icon-m-file-archive-folder"
        onClicked: {
          var dialog = pageStack.replace(Qt.resolvedUrl("RouteDisplayDialog.qml"),
                                         {router: page.router})
          dialog.onAccepted.connect(routeLoader);
        }
        enabled: !page.router.edited
      }

    }

    VerticalScrollDecorator {flickable: flickable}
  }

}
