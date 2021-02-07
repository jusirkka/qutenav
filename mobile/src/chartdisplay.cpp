#include "chartdisplay.h"

#include <QtGui/QOpenGLContext>
#include "chartmanager.h"
#include "textmanager.h"
#include "rastersymbolmanager.h"
#include "vectorsymbolmanager.h"
#include "chartrenderer.h"
#include "conf_mainwindow.h"
#include "conf_marinerparams.h"
#include "detailmode.h"
#include <QQuickWindow>
#include <QOffscreenSurface>
#include "settings.h"

ChartDisplay::ChartDisplay()
  : QQuickFramebufferObject()
  , m_camera(DetailMode::RestoreCamera())
  , m_initialized(false)
  , m_flags(0)
  , m_orientation(Qt::PortraitOrientation)
  , m_scaleBarLength(100)
{
  setMirrorVertically(true);
  connect(this, &QQuickItem::windowChanged, this, &ChartDisplay::handleWindowChanged);
}


void ChartDisplay::handleWindowChanged(QQuickWindow *win) {
  if (!win) return;

  qDebug() << "window changed" << win->size();

  connect(win, &QQuickWindow::openglContextCreated, this, &ChartDisplay::initializeGL, Qt::DirectConnection);
  connect(win, &QQuickWindow::sceneGraphInitialized, this, &ChartDisplay::initializeSG, Qt::DirectConnection);
  connect(win, &QQuickWindow::sceneGraphInvalidated, this, &ChartDisplay::finalizeSG, Qt::DirectConnection);
  connect(win, &QQuickWindow::heightChanged, this, &ChartDisplay::resize, Qt::DirectConnection);
  connect(win, &QQuickWindow::widthChanged, this, &ChartDisplay::resize, Qt::DirectConnection);
  connect(win, &QQuickWindow::contentOrientationChanged, this, &ChartDisplay::orient);
}

void ChartDisplay::finalizeSG() {
  qDebug() << "finalize SG";


  qDebug() << "disconnect";
  this->disconnect();

  auto chartMgr = ChartManager::instance();
  qDebug() << "disconnect chartmgr";
  disconnect(chartMgr, nullptr, this, nullptr);

  auto textMgr = TextManager::instance();
  qDebug() << "disconnect textmgr";
  disconnect(textMgr, nullptr, this, nullptr);

  auto settings = Settings::instance();
  qDebug() << "disconnect settings";
  disconnect(settings, nullptr, this, nullptr);

  Conf::MainWindow::self()->save();
  Conf::MarinerParams::self()->save();
}

void ChartDisplay::initializeSG() {
  qDebug() << "initialize SG";

  auto chartMgr = ChartManager::instance();

  connect(this, &ChartDisplay::updateViewport, chartMgr, &ChartManager::updateCharts);

  connect(chartMgr, &ChartManager::idle, this, [this] () {
    m_flags |= LeavingChartMode;
    update();
  });
  connect(chartMgr, &ChartManager::active, this, [this] () {
    m_flags |= EnteringChartMode;
    update();
  });
  connect(chartMgr, &ChartManager::chartsUpdated, this, [this] (const QRectF& viewArea) {
    m_flags |= ChartsUpdated;
    m_viewArea = viewArea;
    update();
  });

  auto textMgr = TextManager::instance();

  connect(textMgr, &TextManager::newStrings, this, [this] () {
    qDebug() << "new strings";
    emit updateViewport(m_camera, ChartManager::Force);
    update();
  });

  auto settings = Settings::instance();

  connect(settings, &Settings::settingsChanged, this, [this] () {
    qDebug() << "settings changed";
    emit updateViewport(m_camera, ChartManager::Force);
    update();
  });

  connect(settings, &Settings::lookupUpdateNeeded, this, [this] () {
    qDebug() << "lookup update needed";
    emit updateViewport(m_camera, ChartManager::UpdateLookups);
    update();
  });


  emit updateViewport(m_camera, ChartManager::Force);
}

ChartDisplay::~ChartDisplay() {
  delete m_camera;
  delete m_surface;
  delete m_context;
}

ChartDisplay::Renderer* ChartDisplay::createRenderer() const {
  qDebug() << "create renderer";
  return new ChartRenderer(window());
}

QString ChartDisplay::defaultChartSet() const {
  const QStringList sets = chartSets();
  if (sets.empty()) return "None";

  QString s = Conf::MainWindow::chartset();
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


bool ChartDisplay::consume(quint32 flag) {
  const bool ret = (m_flags & flag) != 0;
  m_flags &= ~flag;
  return ret;
}


void ChartDisplay::initializeGL(QOpenGLContext* ctx) {
  if (m_initialized) return;

  m_context = new QOpenGLContext;
  m_context->setFormat(QSurfaceFormat::defaultFormat());
  qDebug() << "share context" << ctx->shareContext();
  m_context->setShareContext(ctx);
  m_context->setScreen(ctx->screen());
  m_context->create();

  m_surface = new QOffscreenSurface;
  m_surface->setFormat(m_context->format());
  m_surface->create();

  m_context->makeCurrent(m_surface);

  m_vao.create();
  m_vao.bind();

  ChartManager::instance()->createThreads(m_context);
  TextManager::instance()->createBuffers();
  RasterSymbolManager::instance()->createSymbols();
  VectorSymbolManager::instance()->createSymbols();

  if (ChartManager::instance()->outlines().isEmpty()) {
    ChartManager::instance()->setChartSet(defaultChartSet(),
                                          m_camera->geoprojection());
  }

  m_initialized = true;
  emit updateViewport(m_camera);
}

void ChartDisplay::resize(int) {

  qDebug() << "resize";

  if (m_size == window()->size()) return;
  m_size = window()->size();
  if (m_size.isEmpty()) return;

  orient(m_orientation);
}

void ChartDisplay::orient(Qt::ScreenOrientation orientation) {
  qDebug() << "orientation" << orientation;
  if (orientation == Qt::LandscapeOrientation) {
    m_orientedSize = QSize(m_size.height(), m_size.width());
  } else if (orientation == Qt::PortraitOrientation) {
    m_orientedSize = m_size;
  } else {
    return;
  }

  m_orientation = orientation;

  if (m_orientedSize.isEmpty()) return;

  const float wmm = m_orientedSize.width() / dots_per_mm_x;
  const float hmm = m_orientedSize.height() / dots_per_mm_y;

  try {
    m_camera->resize(wmm, hmm);
  } catch (ScaleOutOfBounds& e) {
    m_camera->setScale(e.suggestedScale());
    m_camera->resize(wmm, hmm);
  }
  qDebug() << "WxH =" << m_orientedSize.width() << "x" << m_orientedSize.height();
  qDebug() << "WxH (mm) =" << wmm << "x" << hmm;

  emit updateViewport(m_camera);
  computeScaleBar();
  emit scaleBarLengthChanged(m_scaleBarLength);
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
    emit scaleBarLengthChanged(m_scaleBarLength);
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
    emit scaleBarLengthChanged(m_scaleBarLength);
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

void ChartDisplay::setCamera(Camera *cam) {
  delete m_camera;
  m_camera = cam;
  emit updateViewport(m_camera, ChartManager::Force);
  computeScaleBar();
  emit scaleBarLengthChanged(m_scaleBarLength);
}

void ChartDisplay::checkChartSet() const {
  if (ChartManager::instance()->outlines().isEmpty()) {
    ChartManager::instance()->setChartSet(defaultChartSet(), m_camera->geoprojection());
  }
}

static QVector<quint32> digits(quint32 v) {
  // Fails if v = 0, but it's ok
  QVector<quint32> ds;
  while (v != 0) {
    ds.append(v % 10);
    v /= 10;
  }
  return ds;
}


void ChartDisplay::computeScaleBar() {
  // quarter width of the window in meters
  const float w = m_camera->heightMM() * m_camera->aspect();
  const quint32 threshold = 2.5e-4 * w * m_camera->scale();
  auto ds = digits(threshold);
  qreal x = ds[ds.size() - 1] * exp10(ds.size() - 1);
  if (ds.size() > 1 && ds.size() != 4 && ds[ds.size() - 2] >= 5) {
    x += 5 * exp10(ds.size() - 2);
  }
  m_scaleBarLength = dots_per_mm_x * x / m_camera->scale() * 1000;
  if (ds.size() > 3) {
    const int n = static_cast<int>(x) / 1000;
    m_scaleBarText = QString::number(n) + "km";
  } else {
    const int n = static_cast<int>(x);
    m_scaleBarText = QString::number(n) + "m";
  }
  qDebug() << "scalebar:" << m_scaleBarText;
}
