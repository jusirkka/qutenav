/* -*- coding: utf-8-unix -*-
 *
 * File: mobile/src/chartdisplay.cpp
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
#include "chartdisplay.h"

#include "chartmanager.h"
#include "chartrenderer.h"
#include "conf_mainwindow.h"
#include "conf_marinerparams.h"
#include "conf_units.h"
#include "conf_quick.h"
#include "detailmode.h"
#include <QQuickWindow>
#include <QOffscreenSurface>
#include "settings.h"
#include "textmanager.h"
#include "units.h"
#include "logging.h"
#include "dbupdater_interface.h"
#include <QProcess>
#include <QTimer>
#include "platform.h"
#include "rastersymbolmanager.h"
#include "vectorsymbolmanager.h"

ObjectObject::ObjectObject(const QString &n, QObject *parent)
  : QObject(parent)
  , name(n)
{}

AttributeObject::AttributeObject(const QString &n, const QString &v, QObject *parent)
  : QObject(parent)
  , name(n)
  , value(v)
{}

ChartDisplay::ChartDisplay()
  : QQuickFramebufferObject()
  , m_camera(DetailMode::RestoreCamera())
  , m_initialized(false)
  , m_flags(0)
  , m_orientation(Qt::PortraitOrientation)
  , m_scaleBarLength(100)
  , m_scaleBarText {"", "", ""}
  , m_updater(new UpdaterInterface(this))
{
  setMirrorVertically(true);
  connect(this, &QQuickItem::windowChanged, this, &ChartDisplay::handleWindowChanged);
  connect(m_updater, &UpdaterInterface::status, this, &ChartDisplay::handleUpdaterStatus);

  // Ensure that manager instances run in the main thread by calling them here
  ChartManager::instance()->createThreads();
  ChartManager::instance()->setChartSet(defaultChartSet(), m_camera->geoprojection());
  RasterSymbolManager::instance()->init();
  VectorSymbolManager::instance()->init();
  TextManager::instance()->init();
}


void ChartDisplay::handleWindowChanged(QQuickWindow *win) {
  if (!win) return;

  qCDebug(CDPY) << "window changed" << win->size();
  resize();

  connect(win, &QQuickWindow::sceneGraphInitialized, this, &ChartDisplay::initializeSG);
  connect(win, &QQuickWindow::sceneGraphInvalidated, this, &ChartDisplay::finalizeSG, Qt::DirectConnection);
  connect(win, &QQuickWindow::heightChanged, this, &ChartDisplay::resize);
  connect(win, &QQuickWindow::widthChanged, this, &ChartDisplay::resize);
  connect(win, &QQuickWindow::contentOrientationChanged, this, &ChartDisplay::orient);
}

void ChartDisplay::finalizeSG() {
  qCDebug(CDPY) << "finalize SG";


  qCDebug(CDPY) << "disconnect";
  this->disconnect();

  auto chartMgr = ChartManager::instance();
  qCDebug(CDPY) << "disconnect chartmgr";
  disconnect(chartMgr, nullptr, this, nullptr);

  auto textMgr = TextManager::instance();
  qCDebug(CDPY) << "disconnect textmgr";
  disconnect(textMgr, nullptr, this, nullptr);

  auto settings = Settings::instance();
  qCDebug(CDPY) << "disconnect settings";
  disconnect(settings, nullptr, this, nullptr);

  Conf::MainWindow::self()->save();
  Conf::MarinerParams::self()->save();
  Conf::Units::self()->save();
  Conf::Quick::self()->save();
}

void ChartDisplay::initializeSG() {

  qCDebug(CDPY) << "initialize SG";

  auto chartMgr = ChartManager::instance();

  connect(this, &ChartDisplay::updateViewport, chartMgr, &ChartManager::updateCharts);

  connect(this, &ChartDisplay::infoRequest, chartMgr, &ChartManager::requestInfo);
  connect(chartMgr, &ChartManager::infoResponse, this, &ChartDisplay::infoQueryReady);

  connect(chartMgr, &ChartManager::chartSetsUpdated, this, &ChartDisplay::updateChartSet);

  connect(chartMgr, &ChartManager::idle, this, [this] () {
    // qCDebug(CDPY) << "leaving chart mode";
    m_flags |= LeavingChartMode;
    update();
  });

  connect(chartMgr, &ChartManager::active, this, [this] (const QRectF& viewArea) {
    // qCDebug(CDPY) << "active";
    m_flags |= EnteringChartMode | ChartsUpdated;
    m_viewArea = viewArea;
    update();
  });

  connect(chartMgr, &ChartManager::chartsUpdated, this, [this] (const QRectF& viewArea) {
    // qCDebug(CDPY) << "charts updated";
    m_flags |= ChartsUpdated;
    m_viewArea = viewArea;
    update();
  });

  connect(chartMgr, &ChartManager::proxyChanged, this, [this] () {
    // qCDebug(CDPY) << "proxy changed";
    m_flags |= ProxyChanged;
    update();
  });

  if (chartMgr->outlines().isEmpty()) {
    chartMgr->setChartSet(defaultChartSet(), m_camera->geoprojection());
  }

  auto textMgr = TextManager::instance();

  connect(textMgr, &TextManager::newStrings, this, [this] () {
    // qCDebug(CDPY) << "new strings";
    emit updateViewport(m_camera, ChartManager::Force);
  });

  connect(textMgr, &TextManager::newGlyphs, this, [this] () {
    // qCDebug(CDPY) << "new glyphs";
    m_flags |= GlyphAtlasChanged;
    update();
  });


  auto settings = Settings::instance();

  connect(settings, &Settings::settingsChanged, this, [this] () {
    qCDebug(CDPY) << "settings changed";
    emit updateViewport(m_camera, ChartManager::Force);
    computeScaleBar();
  });

  connect(settings, &Settings::lookupUpdateNeeded, this, [this] () {
    qCDebug(CDPY) << "lookup update needed";
    emit updateViewport(m_camera, ChartManager::UpdateLookups);
  });

  connect(Settings::instance(), &Settings::colorTableChanged, this, [this] () {
    qCDebug(CDPY) << "colortable changed";
    m_flags |= ColorTableChanged | ChartsUpdated;
    update();
  });

  emit updateViewport(m_camera, ChartManager::Force);
}

ChartDisplay::~ChartDisplay() {
  delete m_camera;
  qDeleteAll(m_info);
  // This can be done only when exiting
  delete ChartManager::instance();
}

ChartDisplay::Renderer* ChartDisplay::createRenderer() const {
  qCDebug(CDPY) << "create renderer";
  return new ChartRenderer(window());
}

QString ChartDisplay::defaultChartSet() const {
  const QStringList sets = chartSets();
  if (sets.empty()) return "None";

  QString s = Conf::MainWindow::Chartset();
  if (sets.contains(s)) return s;

  s = chartSet();
  Conf::MainWindow::setChartset(s);
  if (sets.contains(s)) return s;

  Conf::MainWindow::setChartset(sets.first());
  return sets.first();
}

QStringList ChartDisplay::chartSets() const {
  return ChartManager::instance()->chartSets();
}

void ChartDisplay::setChartSet(const QString &name) {
  ChartManager::instance()->setChartSet(name, m_camera->geoprojection());
  Conf::MainWindow::setChartset(name);
  m_flags |= ChartSetChanged;
  emit updateViewport(m_camera, ChartManager::Force);
  emit chartSetChanged(name);
  update();
}

QString ChartDisplay::chartSet() const {
  return ChartManager::instance()->chartSet();
}

void ChartDisplay::updateChartSet() {
  const auto name = Conf::MainWindow::Chartset();
  ChartManager::instance()->setChartSet(name, m_camera->geoprojection(), true);
  m_flags |= ChartSetChanged;
  emit updateViewport(m_camera, ChartManager::Force);
  emit chartSetsChanged();
  update();
}

bool ChartDisplay::consume(quint32 flag) {
  const bool ret = (m_flags & flag) != 0;
  m_flags &= ~flag;
  return ret;
}


void ChartDisplay::resize(int) {

  qCDebug(CDPY) << "resize";

  if (window() == nullptr) return;
  if (window()->size().isEmpty()) return;

  if (m_size == window()->size()) return;
  m_size = window()->size();

  qCDebug(CDPY) << "resize->orient";
  orient(m_orientation);
}

void ChartDisplay::orient(Qt::ScreenOrientation orientation) {
  qCDebug(CDPY) << "orientation" << orientation;
  if (orientation == Qt::LandscapeOrientation) {
    m_orientedSize = QSize(m_size.height(), m_size.width());
  } else if (orientation == Qt::PortraitOrientation) {
    m_orientedSize = m_size;
  } else {
    return;
  }

  m_orientation = orientation;

  if (m_orientedSize.isEmpty()) return;

  const float wmm = m_orientedSize.width() / dots_per_mm_x();
  const float hmm = m_orientedSize.height() / dots_per_mm_y();

  try {
    m_camera->resize(wmm, hmm);
  } catch (ScaleOutOfBounds& e) {
    m_camera->setScale(e.suggestedScale());
    m_camera->resize(wmm, hmm);
  }
  qCDebug(CDPY) << "WxH =" << m_orientedSize.width() << "x" << m_orientedSize.height();
  qCDebug(CDPY) << "WxH (mm) =" << wmm << "x" << hmm;

  emit updateViewport(m_camera, ChartManager::Force);
  computeScaleBar();
}


void ChartDisplay::zoomIn() {
  // evenly distributed steps in log scale, div steps per decade
  const float s = m_camera->scale();
  const float s_min = m_camera->minScale();
  const float div = 30.;
  const qint32 i = qint32((log10(s/s_min)) * div + .5) - 1;
  const quint32 scale = s_min * exp10(i / div);

  if (scale > m_camera->minScale()) {
    m_camera->setScale(scale);
    emit updateViewport(m_camera);
    computeScaleBar();
    update();
  }
}

void ChartDisplay::zoomOut() {
  const float s_min = m_camera->minScale();
  const float div = 30;
  const quint32 i = quint32((log10(m_camera->scale() / s_min)) * div + .5) + 1;
  const float scale = s_min * exp10(i / div);

  if (scale < m_camera->maxScale()) {
    m_camera->setScale(scale);
    emit updateViewport(m_camera);
    computeScaleBar();
    update();
  }
}

void ChartDisplay::panStart(qreal x, qreal y) {
  const qreal w = m_orientedSize.width();
  const qreal h = m_orientedSize.height();

  m_lastPos = QPointF(2 * x / w - 1, 1 - 2 * y / h);
}

void ChartDisplay::pan(qreal x, qreal y) {
  const qreal w = m_orientedSize.width();
  const qreal h = m_orientedSize.height();

  const QPointF c(2 * x / w - 1, 1 - 2 * y / h);

  m_camera->pan(m_lastPos, c - m_lastPos);

  m_lastPos = c;
  emit updateViewport(m_camera);
  update();
}


void ChartDisplay::setEye(const QGeoCoordinate& q) {
  if (!q.isValid()) return;
  m_camera->setEye(WGS84Point::fromLL(q.longitude(), q.latitude()));
  emit updateViewport(m_camera);
  update();
}

QPointF ChartDisplay::position(const WGS84Point& wp) const {
  // Normalized device coordinates
  const QPointF pos = m_camera->position(wp);

  const qreal w = m_orientedSize.width();
  const qreal h = m_orientedSize.height();

  return QPointF(w / 2. * (pos.x() + 1), h / 2. * (1 - pos.y()));
}

QPointF ChartDisplay::position(const QGeoCoordinate& q) const {
  if (!q.isValid()) {
    qCWarning(CDPY) << "position: Cannot convert QGeoCoordinate to WGS84Point";
    return position(WGS84Point::fromLL(0, 0));
  }
  return position(WGS84Point::fromLL(q.longitude(), q.latitude()));
}

QPointF ChartDisplay::advance(const QGeoCoordinate& q, qreal distance, qreal heading) const {
  if (!q.isValid()) {
    qCWarning(CDPY) << "advance: Cannot convert QGeoCoordinate to WGS84Point";
    return position(WGS84Point::fromLL(0, 0));
  }
  const auto wp = WGS84Point::fromLL(q.longitude(), q.latitude()) + WGS84Bearing::fromMeters(distance, Angle::fromDegrees(heading));
  return position(wp);
}

void ChartDisplay::syncPositions(const WGS84PointVector& wps, PointVector& vertices) const {

  const float w = m_orientedSize.width();
  const float h = m_orientedSize.height();

  for (int i = 0; i < wps.size(); ++i) {
    // Normalized device coordinates
    const QPointF pos = m_camera->position(wps[i]);
    vertices[i].rx() = w / 2. * (pos.x() + 1);
    vertices[i].ry() = h / 2. * (1 - pos.y());
  }
}

void ChartDisplay::syncPositions(const KV::EventString& events, PointVector& vertices) const {

  const float w = m_orientedSize.width();
  const float h = m_orientedSize.height();

  for (int i = 0; i < events.size(); ++i) {
    // Normalized device coordinates
    const QPointF pos = m_camera->position(events[i].position);
    vertices[i].rx() = w / 2. * (pos.x() + 1);
    vertices[i].ry() = h / 2. * (1 - pos.y());
  }
}

void ChartDisplay::northUp() {
  Angle a = m_camera->northAngle();
  m_camera->rotateEye(- a);
  emit updateViewport(m_camera);
  update();
}

void ChartDisplay::rotate(qreal degrees) {
  Angle a = Angle::fromDegrees(degrees);
  m_camera->rotateEye(a);
  emit updateViewport(m_camera);
  update();
}

WGS84Point ChartDisplay::location(const QPointF& q) const {
  const qreal w = m_orientedSize.width();
  const qreal h = m_orientedSize.height();

  const QPointF p(2 * q.x() / w - 1, 1 - 2 * q.y() / h);
  return m_camera->location(p);
}

QGeoCoordinate ChartDisplay::tocoord(const QPointF& p) const {
  const WGS84Point q = location(p);
  return QGeoCoordinate(q.lat(), q.lng());
}

void ChartDisplay::infoQuery(const QPointF& q) {
  emit infoRequest(location(q));
}

void ChartDisplay::handleFullInfoResponse(const S57::InfoTypeFull &info) {
  qDeleteAll(m_info);
  m_info.clear();
  for (const S57::Description& d: info) {
    auto obj = new ObjectObject(d.name);
    for (const S57::Pair& p: d.attributes) {
      obj->attributes.append(new AttributeObject(p.key, p.value, obj));
    }
    m_info.append(obj);
  }
  emit infoQueryFullReady(m_info);
  update();
}

void ChartDisplay::setCamera(Camera *cam) {
  delete m_camera;
  m_camera = cam;
  emit updateViewport(m_camera, ChartManager::Force);
  computeScaleBar();
}

void ChartDisplay::checkChartSet() const {
  if (ChartManager::instance()->outlines().isEmpty()) {
    ChartManager::instance()->setChartSet(defaultChartSet(), m_camera->geoprojection());
  }
}

static QVector<quint32> digits(quint32 v) {
  QVector<quint32> ds;
  do {
    ds.append(v % 10);
    v /= 10;
  } while (v != 0);

  return ds;
}

void ChartDisplay::computeScaleBar() {
  // quarter width of the window in meters
  const float w = m_camera->heightMM() * m_camera->aspect();
  const float thresholdSI = 2.5e-4 * w * m_camera->scale();
  auto conv = Units::Manager::instance()->distance();
  float threshold = conv->fromSI(thresholdSI);
  if (threshold < 1.) {
    conv = Units::Manager::instance()->shortDistance();
    threshold = conv->fromSI(thresholdSI);
  }
  auto ds = digits(threshold);
  float x = ds[ds.size() - 1] * exp10(ds.size() - 1);
  if (ds.size() > 1 && ds[ds.size() - 2] >= 5) {
    x += 5 * exp10(ds.size() - 2);
  }
  const quint32 sc = m_camera->scale();
  m_scaleBarLength = dots_per_mm_x() * conv->toSI(x) / sc * 1000;
  m_scaleBarText[0] = conv->display(x);
  m_scaleBarText[1] = QString("1:%1").arg(static_cast<quint32>(1000 * std::round(sc / 1000.)));
  m_scaleBarText[2] = "";
  if (Conf::Quick::IndicateScales() && ChartManager::instance()->nextScale() > 0) {
    m_scaleBarText[2] = QString("1:%1").arg(ChartManager::instance()->nextScale());
  }
  // qCDebug(CDPY) << "scalebar:" << m_scaleBarText;
  emit scaleBarLengthChanged(m_scaleBarLength);
  emit scaleBarTextChanged();
}

QString ChartDisplay::scaleBarText(int index) const {
  if ((index < 0) || (index > 2)) return "";
  return m_scaleBarText[index];
}

QString ChartDisplay::displayBearing(const QGeoCoordinate& q1, const QGeoCoordinate& q2, bool swap) const {
  if (!q1.isValid() || !q2.isValid()) return "";

  const WGS84Bearing d = WGS84Point::fromLL(q2.longitude(), q2.latitude()) - WGS84Point::fromLL(q1.longitude(), q1.latitude());

  auto conv = Units::Manager::instance()->distance();
  if (conv->fromSI(d.meters()) < 1.) {
    conv = Units::Manager::instance()->shortDistance();
  }

  int b = std::round(d.degrees());

  if (b < 0) b += 360;
  int b1 = (b + 180) % 360;
  if (swap) {
    const int tmp = b;
    b = b1;
    b1 = tmp;
  }

  const QString tmpl = "%1   %2° - %3°";
  const QChar z('0');

  return tmpl.arg(conv->displaySI(d.meters(), 3)).arg(b, 3, 10, z).arg(b1, 3, 10, z);
}

#include <QTextStream>

void ChartDisplay::advanceNMEALog(int secs) const {
  QFile f("/tmp/nmea.log");
  if (!f.open(QIODevice::ReadWrite | QIODevice::Text)) return;

  QString s;
  QTextStream stream(&f);
  while (!stream.atEnd() && secs > 0) {
    stream.readLine();
    secs--;
  }

  while (!stream.atEnd()) {
    const QString line = stream.readLine();
    s.append(line + "\n");
  }

  f.resize(0);
  stream << s;
  f.close();
}

void ChartDisplay::updateChartDB(bool fullUpdate) {
  qCDebug(CDPY) << "updateChartDB" << m_updater;
  auto resp = m_updater->ping();
  qCDebug(CDPY) << "pong:" << resp.isValid();
  if (!resp.isValid()) {
    qCDebug(CDPY) << resp.error().name() << resp.error().message();
    qCDebug(CDPY) << "Launching dbupdater";
    auto updater = QString("%1_dbupdater").arg(qAppName().replace("-", "_"));
    bool ok = QProcess::startDetached(updater, QStringList());
    qCDebug(CDPY) << "Launched" << updater << ", status =" << ok;
    if (ok) {
      if (fullUpdate) {
        QTimer::singleShot(1000, this, &ChartDisplay::requestChartDBFullUpdate);
      } else {
        QTimer::singleShot(1000, this, &ChartDisplay::requestChartDBUpdate);
      }
    }
  } else {
    if (fullUpdate) {
      requestChartDBFullUpdate();
    } else {
      requestChartDBUpdate();
    }
  }
}

void ChartDisplay::requestChartDBUpdate() {
  m_updater->sync(Conf::MainWindow::ChartFolders());
}


void ChartDisplay::requestChartDBFullUpdate() {
  m_updater->fullSync(Conf::MainWindow::ChartFolders());
}

void ChartDisplay::handleUpdaterStatus(const QString& msg) {
  auto parts = msg.split(":");
  QString statusString;
  bool ok = true;
  if (parts[0] == "Ready") {
    if (parts[1] == "FullSync") {
      statusString = qtTrId(status_full_sync_ready);
    } else if (parts[1] == "Sync") {
      statusString = qtTrId(status_sync_ready);
    } else {
      qCWarning(CDPY) << "received unknown status from DBUpdater" << msg;
      ok = false;
    }
  } else if (parts[0] == "Insert") {
    auto n = parts[1].toInt();
    auto m = parts[2].toInt();
    statusString = qtTrId(status_insert_1, n) + qtTrId(status_insert_2, m);
  } else if (parts[0] == "Update") {
    auto n = parts[1].toInt();
    auto m = parts[2].toInt();
    statusString = qtTrId(status_update_1, n) + qtTrId(status_update_2, m);
  } else {
    qCWarning(CDPY) << "received unknown status from DBUpdater" << msg;
    ok = false;
  }
  if (ok) {
    emit chartDBStatus(statusString);
  }
}
