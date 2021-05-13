import QtQuick 2.12

DialogPL {
  id: dialog
  title: "Chart folders"
  hasOK: true
  pageHeight: parent.height * .6
  pageWidth: parent.width * .5

  property var paths: []
  property bool fullUpdate: false

  ButtonPL {
    id: addButton
    text: "Add"
    width: 100
    anchors {
      top: parent.top
      right: parent.right
      leftMargin: theme.paddingSmall
      rightMargin: theme.paddingSmall
      bottomMargin: theme.paddingSmall
    }
    onClicked: {
      var component = Qt.createComponent(Qt.resolvedUrl("ChartFolderSelector.qml"));
      var sel = component.createObject(dialog, {});
      sel.onAccepted.connect(function () {
        var path = ("" + sel.fileUrl).substring(7)
        dialog.paths.push(path)
        pathsView.model = dialog.paths
      });
    }
  }

  ButtonPL {
    id: delButton
    text: "Remove"
    width: addButton.width
    enabled: pathsView.currentIndex >= 0
    anchors {
      top: addButton.bottom
      right: parent.right
      leftMargin: theme.paddingSmall
      rightMargin: theme.paddingSmall
      topMargin: theme.paddingSmall
    }
    onClicked: {
      dialog.paths.splice(pathsView.currentIndex, 1)
      pathsView.currentIndex = -1
      pathsView.model = dialog.paths
    }
  }

  Rectangle {
    id: pathFrame
    height: 6 * 30
    anchors {
      top: parent.top
      left: parent.left
      right: addButton.left
      leftMargin: theme.paddingSmall
      rightMargin: theme.paddingSmall
    }
    border.color: "black"
    border.width: 1

    ListViewPL {
      id: pathsView
      anchors.fill: parent
      model: dialog.paths
      delegate: Text {
        width: pathsView.width
        text: modelData
        elide: Text.ElideLeft
        MouseArea {
          anchors.fill: parent
          onClicked: pathsView.currentIndex = index
        }
      }
    }

  }

  TextSwitchPL {
    text: "Full update"
    description: "Check also previously added chart folders for changes"
    anchors {
      top: pathFrame.bottom
      left: parent.left
      topMargin: theme.paddingSmall
      leftMargin: theme.paddingSmall
      rightMargin: theme.paddingSmall
    }
    onCheckedChanged: {
      dialog.fullUpdate = checked;
    }
  }
}
