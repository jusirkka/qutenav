import QtQuick 2.15
import QtQuick.Window 2.2

Item {

  readonly property int fontSizeMedium: Math.round(Qt.application.font.pixelSize * 1.0)

  readonly property int fontSizeHuge: Math.round(fontSizeMedium * 3.0)
  readonly property int fontSizeExtraLarge: Math.round(fontSizeMedium * 2.0)
  readonly property int fontSizeLarge: Math.round(fontSizeMedium * 1.5)
  readonly property int fontSizeSmall: Math.round(fontSizeMedium * .9)
  readonly property int fontSizeExtraSmall: Math.round(fontSizeMedium * .7)

  readonly property real horizontalPageMargin: .75 * fontSizeMedium
  readonly property real paddingLarge: .75 * fontSizeMedium
  readonly property real paddingMedium: .5 * fontSizeMedium
  readonly property real paddingSmall: .25 * fontSizeMedium

  readonly property color highlightColor: palette.windowText
  readonly property color primaryColor: palette.text
  readonly property color secondaryHighlightColor: inactivePalette.text
  readonly property color secondaryColor: inactivePalette.text

  readonly property int itemSizeLarge: fontSizeLarge * 3
  readonly property int itemSizeMedium: fontSizeMedium * 3
  readonly property int itemSizeSmall: fontSizeSmall * 3
  readonly property int itemSizeExtraSmall: fontSizeExtraSmall * 3

  readonly property real pixelRatio: Screen.devicePixelRatio

  readonly property real mapButtonIconSize: 24 * pixelRatio
  readonly property real bubbleTextFontSize: fontSizeMedium


  function highlightText(base, part, hicolor) {
    if (part === "") return base
    var mo = new RegExp(part, 'i')
    var parts = base.match(mo) // first match
    if (!parts) return base
    return base.replace(mo, "<font color='" + hicolor + "'>" + parts[0] + "</font>")
  }

  SystemPalette {
    id: palette
    colorGroup: SystemPalette.Active
  }

  SystemPalette {
    id: inactivePalette
    colorGroup: SystemPalette.Inactive
  }
}
