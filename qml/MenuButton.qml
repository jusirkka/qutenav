/* -*- coding: utf-8-unix -*-
 *
 * File: mobile/qml/MenuButton.qml
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

MapButtonPL {
  id: button

  anchors.bottom: parent.bottom
  anchors.bottomMargin: theme.paddingMedium
  anchors.rightMargin: theme.paddingMedium
  anchors.leftMargin: theme.paddingMedium

  property var routeLoader: undefined


  iconSource: app.getIcon("menu")

  onClicked: app.show(Qt.resolvedUrl("MenuPage.qml"),
                      {tracker: tracker,
                        router: router,
                        routeLoader: routeLoader,
                      });

}
