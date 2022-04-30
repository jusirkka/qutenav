/* -*- coding: utf-8-unix -*-
 *
 * File: ApplicationWindowPL.qml
 *
 * Copyright (C) 2021 Jukka Sirkka
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
import QtQuick 2.6

import "./platform"

DialogPL {
  id: dialog
  //% "Chart folders"
  title: qsTrId("qutenav-dialog-chart-folders")
  hasOK: true
  pageHeight: parent.height * .7

  property var paths
  property bool fullUpdate: false

  onAccepted: {
    settings.chartFolders = paths
    app.encdis.updateChartDB(fullUpdate)
  }

  Rectangle {
    id: pathFrame
    height: 3 * theme.itemSizeMedium
    anchors {
      top: parent.top
      left: parent.left
      right: parent.right
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
        font.pixelSize: theme.fontSizeMedium
        MouseArea {
          anchors.fill: parent
          onClicked: pathsView.currentIndex = index
        }
      }
    }
  }

  Row {

    id: buttons

    spacing: theme.paddingMedium

    anchors {
      top: pathFrame.bottom
      horizontalCenter: parent.horizontalCenter
      topMargin: theme.paddingMedium
      leftMargin: theme.paddingSmall
      rightMargin: theme.paddingSmall
      bottomMargin: theme.paddingMedium
    }

    ButtonPL {
      id: addButton
      //% "Add"
      text: qsTrId("qutenav-dialog-add")
      width: delButton.width
      onClicked: {
        var picker = app.showAsDialog(Qt.resolvedUrl("ChartFolderSelector.qml"))
        picker.onAccepted.connect(function () {
          var path = ("" + picker.fileUrl).replace("file://", "")
          console.log("add", path)
          dialog.paths.push(path)
          pathsView.model = dialog.paths
        });
      }
    }

    ButtonPL {
      id: delButton
      //% "Remove"
      text: qsTrId("qutenav-dialog-remove")
      enabled: pathsView.currentIndex >= 0
      onClicked: {
        dialog.paths.splice(pathsView.currentIndex, 1)
        pathsView.currentIndex = -1
        pathsView.model = dialog.paths
      }
    }

  }

  TextSwitchPL {
    //% "Full update"
    text: qsTrId("qutenav-dialog-full-update")
    //% "Check also previously added chart folders for changes"
    description: qsTrId("qutenav-dialog-full-update-description")
    anchors {
      top: buttons.bottom
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
