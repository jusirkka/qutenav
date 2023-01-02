import QtQuick 2.6
import Sailfish.Silica 1.0

Item {

  readonly property int fontSizeMedium: Theme.fontSizeMedium

  readonly property int fontSizeHuge: Theme.fontSizeHuge
  readonly property int fontSizeExtraLarge: Theme.fontSizeExtraLarge
  readonly property int fontSizeLarge: Theme.fontSizeLarge
  readonly property int fontSizeSmall: Theme.fontSizeSmall
  readonly property int fontSizeExtraSmall: Theme.fontSizeExtraSmall

  readonly property real horizontalPageMargin: Theme.horizontalPageMargin
  readonly property real paddingLarge: Theme.paddingLarge
  readonly property real paddingMedium: Theme.paddingMedium
  readonly property real paddingSmall: Theme.paddingSmall

  readonly property color highlightColor: Theme.highlightColor
  readonly property color primaryColor: Theme.primaryColor
  readonly property color secondaryHighlightColor: Theme.secondaryHighlightColor
  readonly property color secondaryColor: Theme.secondaryColor

  readonly property int itemSizeLarge: Theme.itemSizeLarge
  readonly property int itemSizeMedium: Theme.itemSizeMedium
  readonly property int itemSizeSmall: Theme.itemSizeSmall
  readonly property int itemSizeExtraSmall: Theme.itemSizeExtraSmall

  readonly property real pixelRatio: Theme.pixelRatio

  readonly property real mapButtonIconSize: Theme.iconSizeExtraSmall
  readonly property real bubbleTextFontSize: Theme.fontSizeExtraSmall

}
