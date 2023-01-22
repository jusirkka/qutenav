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

import QtQuick 2.6
import "./platform"

PagePL {

  //% "Units"
  title: qsTrId("qtnav-units")
  pageHeight: parent.height

  SectionHeaderPL {
    //% "Location"
    text: qsTrId("qtnav-units-location")
  }

  ComboBoxPL {
    id: locBox
    Component.onCompleted: {
      currentIndex = settings.locationUnits;
      var names = [];
      for (var i = 0; i < settings.locationUnitNames.length; i++) {
        names.push(qsTrId(settings.locationUnitNames[i]));
      }
      model = names;
    }
    onCurrentIndexChanged: {
      settings.locationUnits = currentIndex;
    }
  }

  SectionHeaderPL {
    //% "Depth"
    text: qsTrId("qtnav-units-depth")
  }

  ComboBoxPL {
    id: depthBox
    Component.onCompleted: {
      currentIndex = settings.depthUnits;
      var names = [];
      for (var i = 0; i < settings.depthUnitNames.length; i++) {
        names.push(qsTrId(settings.depthUnitNames[i]));
      }
      model = names;
    }
    onCurrentIndexChanged: {
      settings.depthUnits = currentIndex;
    }
  }

  SectionHeaderPL {
    //% "Distance"
    text: qsTrId("qtnav-units-distance")
  }

  ComboBoxPL {
    id: distanceBox
    Component.onCompleted: {
      currentIndex = settings.distanceUnits;
      var names = [];
      for (var i = 0; i < settings.distanceUnitNames.length; i++) {
        names.push(qsTrId(settings.distanceUnitNames[i]));
      }
      model = names;
    }
    onCurrentIndexChanged: {
      settings.distanceUnits = currentIndex;
    }
  }

  SectionHeaderPL {
    //% "Short Distance"
    text: qsTrId("qtnav-units-sdistance")
  }

  ComboBoxPL {
    id: sdistanceBox
    Component.onCompleted: {
      currentIndex = settings.shortDistanceUnits;
      var names = [];
      for (var i = 0; i < settings.shortDistanceUnitNames.length; i++) {
        names.push(qsTrId(settings.shortDistanceUnitNames[i]));
      }
      model = names;
    }
    onCurrentIndexChanged: {
      settings.shortDistanceUnits = currentIndex;
    }
  }

  SectionHeaderPL {
    //% "Height"
    text: qsTrId("qtnav-units-height")
  }

  ComboBoxPL {
    id: heightBox
    Component.onCompleted: {
      currentIndex = settings.heightUnits;
      var names = [];
      for (var i = 0; i < settings.heightUnitNames.length; i++) {
        names.push(qsTrId(settings.heightUnitNames[i]));
      }
      model = names;
    }
    onCurrentIndexChanged: {
      settings.heightUnits = currentIndex;
    }
  }


  SectionHeaderPL {
    //% "Boat Speed"
    text: qsTrId("qtnav-units-bspeed")
  }

  ComboBoxPL {
    id: bspeedBox
    Component.onCompleted: {
      currentIndex = settings.boatSpeedUnits;
      var names = [];
      for (var i = 0; i < settings.boatSpeedUnitNames.length; i++) {
        names.push(qsTrId(settings.boatSpeedUnitNames[i]));
      }
      model = names;
    }
    onCurrentIndexChanged: {
      settings.boatSpeedUnits = currentIndex;
    }
  }

}
