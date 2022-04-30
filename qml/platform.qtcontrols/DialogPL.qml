/* -*- coding: utf-8-unix -*-
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

import QtQuick 2.9
import QtQuick.Controls 2.15

Dialog {
  id: dialog

  //% "Accept"
  property string acceptText: qsTrId("qutenav-dialog-accept")
  property int pageHeight: 300
  property int pageWidth: parent.width / 3
  property bool hasOK: false

  visible: true
  modal: true
  width: pageWidth
  height: pageHeight

  anchors.centerIn: parent

  onAccepted: {
    close();
  }

  function textTricks(txt) {
    return txt.replace("&", "&&");
  }


  standardButtons: hasOK ? (Dialog.Ok | Dialog.Cancel) : Dialog.Cancel
}
