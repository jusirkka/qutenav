/* -*- coding: utf-8-unix -*-
 *
 * File: mobile/qml/TrackDisplayDialog.qml
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
import QtQuick 2.0
import Sailfish.Silica 1.0
import org.qopencpn 1.0

Dialog {
  id: dialog


  SilicaFlickable {
    id: flickable
    anchors.fill: parent

    DialogHeader {
      id: header
      title: "Select tracks to show"
      acceptText: "Show"
      cancelText: "Cancel"
    }

    SilicaListView {
      id: tracks

      anchors {
        top: header.bottom
        bottom: parent.bottom
        left: parent.left
        right: parent.right
        topMargin: Theme.paddingLarge
      }

      model: TrackModel {}

      delegate: TextSwitch {

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.leftMargin: Theme.horizontalPageMargin
        anchors.rightMargin: Theme.horizontalPageMargin

        Component.onCompleted: {
          checked = model.enabled;
          text = model.name
        }
        onCheckedChanged: {
          model.enabled = checked;
        }
      }

      ViewPlaceholder {
        enabled: tracks.model.count === 0
        text: "No tracks."
        hintText: "Tap tracks record button in Chart view to record a track."
      }
    }

    VerticalScrollDecorator {flickable: flickable}
  }
}
