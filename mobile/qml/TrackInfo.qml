import QtQuick 2.0
import Sailfish.Silica 1.0

Row {

  id: row

  spacing: Theme.paddingSmall

  anchors.top: parent.top
  anchors.left: parent.left

  TrackSpeedInfoBox {
    speed: tracker.speed
    seconds: tracker.duration
    dist: tracker.distance
    bearing: tracker.bearing
  }

  TrackPointInfoBox {
    index: tracker.segmentEndPoint
    seconds: tracker.segmentETA
    dist: tracker.segmentDTG
    bearing: tracker.segmentBearing
    visible: !router.empty && !router.edited
  }

  TrackTargetInfoBox {
    seconds: tracker.targetETA
    dist: tracker.targetDTG
    visible: !router.empty && !router.edited
  }
}
