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

import QtQuick 2.15

PagePL {

  title: "Preferences"
  pageHeight: parent.height

  SectionHeaderPL {
    text: "Symbols"
  }

  TextSwitchPL {
    id: sws
    text: "Simplified symbols"
    description: "Select simplified instead of paperchart symbols."
    Component.onCompleted: {
      sws.checked = settings.simplifiedSymbols;
    }
    onCheckedChanged: {
      settings.simplifiedSymbols = checked;
    }
  }

  TextSwitchPL {
    id: swp
    text: "Plain boundaries"
    description: "Select plain instead of symbolized boundaries of areas."
    Component.onCompleted: {
      swp.checked = settings.plainBoundaries;
    }
    onCheckedChanged: {
      settings.plainBoundaries = checked;
    }
  }

  TextSwitchPL {
    id: swf
    text: "Full sector lengths"
    description: "Show light sectors at their nominal visibility."
    Component.onCompleted: {
      swf.checked = settings.fullLengthSectors;
    }
    onCheckedChanged: {
      settings.fullSectors = checked;
    }
  }

  SectionHeaderPL {
    text: "Chart Object Filter"
  }

  ComboBoxPL {
    id: catBox
    label: "Category"
    description: "Select maximal category to display objects. " +
                 "Selected category includes the previous ones in the list."
    model: settings.maxCategoryNames
    Component.onCompleted: {
      catBox.currentIndex = settings.maxCategory;
    }
    onCurrentIndexChanged: {
      settings.maxCategory = catBox.currentIndex;
    }
  }

  TextSwitchPL {
    id: swm
    text: "Meta objects"
    description: "Show meta objects."
    Component.onCompleted: {
      swm.checked = settings.showMeta;
    }
    onCheckedChanged: {
      settings.showMeta = checked;
    }
  }

  SectionHeaderPL {
    text: "Depths & Contours"
  }

  ValuePref {
    id: sp1
    label: "Safety depth"
    description: "Soundings shallower than safety depth are highlighted."
    Component.onCompleted: {
      sp1.value = settings.safetyDepth;
    }
    onValueChanged: {
      settings.safetyDepth = value;
    }
  }

  ValuePref {
    id: sp2
    label: "Safety contour"
    description: "Depth contour shallower than safety contour is highlighted."
    Component.onCompleted: {
      sp2.value = settings.safetyContour;
    }
    onValueChanged: {
      settings.safetyContour = value;
    }
  }

  ValuePref {
    id: sp3
    label: "Shallow water contour"
    description: "Select shallow water contour for depth area coloring."
    Component.onCompleted: {
      sp3.value = settings.shallowContour;
    }
    onValueChanged: {
      settings.shallowContour = value;
    }
  }

  ValuePref {
    id: sp4
    label: "Deep water contour"
    description: "Select deep water contour for depth area coloring."
    Component.onCompleted: {
      sp4.value = settings.deepContour;
    }
    onValueChanged: {
      settings.deepContour = value;
    }
  }

  TextSwitchPL {
    id: sw2
    text: "Two shades"
    description: "Use just two shades for depth area coloring."
    Component.onCompleted: {
      sw2.checked = settings.twoShades;
    }
    onCheckedChanged: {
      settings.twoShades = checked;
    }
  }

  SectionHeaderPL {
    text: "Colors"
  }

  ComboBoxPL {
    id: colorBox
    label: "Colortable"
    description: "Select colortable for chart colors."
    model: settings.colorTableNames

    Component.onCompleted: {
      colorBox.currentIndex = settings.colorTable;
    }
    onCurrentIndexChanged: {
      settings.colorTable = colorBox.currentIndex;
    }
  }

  SectionHeaderPL {
    text: "Text"
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
