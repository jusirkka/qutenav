import QtQuick 2.2
import Sailfish.Silica 1.0
import QtPositioning 5.2

ApplicationWindow {
  id: app

  initialPage: Component {ChartPage {}}
  cover: Component {CoverPage {}}

  property var encdis: null
  property int pixelRatio: 100

  // for testing
  property int startInstant

  PositionSource {
    id: gps
    nmeaSource: "/tmp/nmea.log"
  }

  Component.onCompleted: {
    setPixelRatio()
    gps.start()
    startInstant = Date.now() / 1000;
  }

  Component.onDestruction: {
    encdis.advanceNMEALog(Date.now() / 1000 - startInstant);
  }

  function setPixelRatio() {
    // Return path to icon suitable for user's screen,
    // finding the closest match to Theme.pixelRatio.
    var ratios = [1.00, 1.25, 1.50, 1.75, 2.00]
    var minIndex = -1, minDiff = 1000, diff
    for (var i = 0; i < ratios.length; i++) {
      diff = Math.abs(Theme.pixelRatio - ratios[i]);
      minIndex = diff < minDiff ? i : minIndex;
      minDiff = Math.min(minDiff, diff);
    }
    app.pixelRatio = Math.floor(100 * ratios[minIndex])
  }

  function getIcon(name) {
    return "icons/%1-%2.png".arg(name).arg(app.pixelRatio);
  }


}


