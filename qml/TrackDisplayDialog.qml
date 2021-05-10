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
import QtQuick 2.6
import org.qutenav 1.0

DialogPL {
  id: dialog

  title: "Select tracks to show"
  acceptText: "Show"
  pageHeight: parent.height * .75
  hasOK: true

  ListViewPL {
    id: tracks

    anchors.fill: parent

    model: TrackModel {}

    delegate: SwitchDelegatePL {

      Component.onCompleted: {
        checked = model.enabled;
        text = model.name
        console.log(text, checked)
      }
      onCheckedChanged: {
        model.enabled = checked;
      }
    }

    ViewPlaceholderPL {
      enabled: tracks.model.count === 0
      text: "No tracks."
      hintText: "Tap tracks record button in Chart view to record a track."
    }
  }
}


