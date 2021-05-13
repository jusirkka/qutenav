import QtQuick 2.15
import QtQuick.Dialogs 1.3

FileDialog {
  id: fileDialog
  title: "Select chart folder"
  folder: shortcuts.home
  selectFolder: true
  Component.onCompleted: {
    visible = true
  }
}
