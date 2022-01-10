import QtQuick 2.6

import "./platform"

DialogPL {
  id: dialog

  property string name

  //% "Edit name"
  title: qsTrId("qtnav-renamer-title")
  pageHeight: parent.height * .75
  //% "Rename"
  acceptText: qsTrId("qutenav-renamer-accept-text")
  hasOK: true

  TextFieldPL {
    id: field
    anchors.bottomMargin: theme.paddingMedium
    height: implicitHeight
    width: parent.width
    font.pixelSize: theme.fontSizeMedium
    color: theme.highlightColor
    Component.onCompleted: {
      text = dialog.name;
    }
    onTextChanged: {
      dialog.name = field.text
    }
  }
}
