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

  //% "Chart"
  title: qsTrId("qtnav-chart")
  pageHeight: parent.height

  SectionHeaderPL {
    //% "Symbols"
    text: qsTrId("qtnav-symbols")
  }

  TextSwitchPL {
    id: sws
    //% "Simplified symbols"
    text: qsTrId("qtnav-simplified-symbols")
    //% "Select simplified instead of paperchart symbols."
    description: qsTrId("qtnav-simplified-symbols-description")
    Component.onCompleted: {
      sws.checked = settings.simplifiedSymbols;
    }
    onCheckedChanged: {
      settings.simplifiedSymbols = checked;
    }
  }

  TextSwitchPL {
    id: swp
    //% "Plain boundaries"
    text: qsTrId("qt-nav-plain-boundaries")
    //% "Select plain instead of symbolized boundaries of areas."
    description: qsTrId("qt-nav-plain-boundaries-description")
    Component.onCompleted: {
      swp.checked = settings.plainBoundaries;
    }
    onCheckedChanged: {
      settings.plainBoundaries = checked;
    }
  }

  TextSwitchPL {
    id: swf
    //% "Full sector lengths"
    text: qsTrId("qtnav-full-sector-lengths")
    //% "Show light sectors at their nominal visibility."
    description: qsTrId("qtnav-full-sector-lengths-description")
    Component.onCompleted: {
      swf.checked = settings.fullLengthSectors;
    }
    onCheckedChanged: {
      settings.fullSectors = checked;
    }
  }

  SectionHeaderPL {
    //% "Chart Object Filter"
    text: qsTrId("qtnav-chart-object-filter")
  }

  ComboBoxPL {
    id: catBox
    //% "Category"
    label: qsTrId("qtnav-category")
    //% "Select maximal category to display objects. Selected category includes the previous ones in the list."
    description: qsTrId("qtnav-category-description")
    Component.onCompleted: {
      catBox.currentIndex = settings.maxCategory;
      var names = [];
      for (var i = 0; i < settings.maxCategoryNames.length; i++) {
        names.push(qsTrId(settings.maxCategoryNames[i]));
      }
      model = names;
    }
    onCurrentIndexChanged: {
      settings.maxCategory = catBox.currentIndex;
    }
  }

  SectionHeaderPL {
    //% "Optional object classes"
    text: qsTrId("qtnav-optional-object-classes")
  }

  Repeater {
    model: settings.optionalClasses
    TextSwitchPL {
      Component.onCompleted: {
        checked = !modelData.enabled;
        text = modelData.text
        description = modelData.description
      }
      onCheckedChanged: {
        modelData.enabled = !checked;
      }
    }
  }

  SectionHeaderPL {
    //% "Minimum scale defaults"
    text: qsTrId("qtnav-scamin-defaults")
  }

  Repeater {
    model: settings.scaminClasses
    IntPref {
      Component.onCompleted: {
        value = modelData.value
        label = modelData.text
        description = modelData.description
      }
      onValueChanged: {
        modelData.value = value
      }
    }
  }

  SectionHeaderPL {
    //% "Depths & Contours"
    text: qsTrId("qtnav-depths-contours")
  }

  ValuePref {
    id: sp1
    //% "Safety depth"
    label: qsTrId("qtnav-safety-depth")
    //% "Soundings shallower than safety depth are highlighted."
    description: qsTrId("qtnav-safety-depth-description")
    symbol: units.depthSymbol
    Component.onCompleted: {
      sp1.value = units.depth(settings.safetyDepth)
    }
    onValueChanged: {
      settings.safetyDepth = units.depthSI(value)
    }
  }

  ValuePref {
    id: sp2
    //% "Safety contour"
    label: qsTrId("qtnav-safety-contour")
    //% "Depth contour shallower than safety contour is highlighted."
    description: qsTrId("qtnav-safety-contour-description")
    symbol: units.depthSymbol
    Component.onCompleted: {
      sp2.value = units.depth(settings.safetyContour)
    }
    onValueChanged: {
      settings.safetyContour = units.depthSI(value)
    }
  }

  ValuePref {
    id: sp3
    //% "Shallow water contour"
    label: qsTrId("qtnav-shallow-water-contour")
    //% "Select shallow water contour for depth area coloring."
    description: qsTrId("qtnav-shallow-water-contour-description")
    symbol: units.depthSymbol
    Component.onCompleted: {
      sp3.value = units.depth(settings.shallowContour)
    }
    onValueChanged: {
      settings.shallowContour = units.depthSI(value)
    }
  }

  ValuePref {
    id: sp4
    //% "Deep water contour"
    label: qsTrId("qtnav-deep-water-contour")
    //% "Select deep water contour for depth area coloring."
    description: qsTrId("qtnav-deep-water-contour-description")
    symbol: units.depthSymbol
    Component.onCompleted: {
      sp4.value = units.depth(settings.deepContour)
    }
    onValueChanged: {
      settings.deepContour = units.depthSI(value)
    }
  }

  TextSwitchPL {
    id: sw2
    //% "Two shades"
    text: qsTrId("qtnav-two-shades")
    //% "Use just two shades for depth area coloring."
    description: qsTrId("qtnav-two-shades-description")
    Component.onCompleted: {
      sw2.checked = settings.twoShades;
    }
    onCheckedChanged: {
      settings.twoShades = checked;
    }
  }

  TextSwitchPL {
    id: sw3
    //% "Shallow Pattern"
    text: qsTrId("qtnav-shallow-pattern")
    //% "Fill depth areas shallower than the safety contour with a pattern."
    description: qsTrId("qtnav-shallow-pattern-description")
    Component.onCompleted: {
      sw3.checked = settings.shallowPattern;
    }
    onCheckedChanged: {
      settings.shallowPattern = checked;
    }
  }

  SectionHeaderPL {
    //% "Colors"
    text: qsTrId("qtnav-colors")
  }

  ComboBoxPL {
    id: colorBox
    //% "Colortable"
    label: qsTrId("qtnav-colortable")
    //% "Select colortable for chart colors."
    description: qsTrId("qtnav-colortable-description")

    Component.onCompleted: {
      colorBox.currentIndex = settings.colorTable;
      var names = [];
      for (var i = 0; i < settings.colorTableNames.length; i++) {
        names.push(qsTrId(settings.colorTableNames[i]));
      }
      model = names;
    }
    onCurrentIndexChanged: {
      settings.colorTable = colorBox.currentIndex;
    }
  }

  SectionHeaderPL {
    //% "Text"
    text: qsTrId("qtnav-text")
  }

  Repeater {
    model: settings.textGroups
    TextSwitchPL {
      Component.onCompleted: {
        checked = modelData.enabled;
        text = modelData.text
        description = modelData.description
      }
      onCheckedChanged: {
        modelData.enabled = checked;
      }
    }
  }
}
