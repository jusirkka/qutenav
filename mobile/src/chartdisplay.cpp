#include "chartdisplay.h"

#include <QtQuick/qquickwindow.h>
#include <QtGui/QOpenGLContext>
#include "outlinemode.h"
#include "chartmanager.h"
#include "textmanager.h"
#include "glcontext.h"
#include "rastersymbolmanager.h"
#include "vectorsymbolmanager.h"
#include "chartrenderer.h"
#include "conf_mainwindow.h"
#include <QSGSimpleTextureNode>

ChartDisplay::ChartDisplay()
  : QQuickItem()
  , m_renderer(nullptr)
  , m_camera(nullptr)
  , m_initialized(false)
  , m_leavingChartMode(false)
  , m_enteringChartMode(false)
  , m_chartsUpdated(false)
  , m_chartSetChanged(false)
{
  setFlag(ItemHasContents, true);

  connect(this, &QQuickItem::windowChanged, this, &ChartDisplay::handleWindowChanged);

  auto chartMgr = ChartManager::instance();
  connect(this, &ChartDisplay::updateViewport, chartMgr, &ChartManager::updateCharts);
  connect(chartMgr, &ChartManager::idle, this, [this] () {
    m_leavingChartMode = true;
    update();
  });
  connect(chartMgr, &ChartManager::active, this, [this] () {
    m_enteringChartMode = true;
    update();
  });
  connect(chartMgr, &ChartManager::chartsUpdated, this, [this] (const QRectF& viewArea) {
    m_chartsUpdated = true;
    m_viewArea = viewArea;
    update();
  });

  auto textMgr = TextManager::instance();
  connect(textMgr, &TextManager::newStrings, this, [this] () {
    qDebug() << "new strings";
    emit updateViewport(m_camera, true);
    update();
  });
}


void ChartDisplay::handleWindowChanged(QQuickWindow *win) {
  if (!win) return;

  qDebug() << "window changed" << win->size();

  connect(win, &QQuickWindow::sceneGraphInvalidated, this, &ChartDisplay::cleanup, Qt::DirectConnection);
  connect(win, &QQuickWindow::sceneGraphInitialized, this, &ChartDisplay::initializeGL, Qt::DirectConnection);
  connect(win, &QQuickWindow::heightChanged, this, &ChartDisplay::resize, Qt::DirectConnection);
  connect(win, &QQuickWindow::widthChanged, this, &ChartDisplay::resize, Qt::DirectConnection);

  if (!m_renderer) {
    m_renderer = new ChartRenderer(win);
  }

  if (!m_camera) {
    m_camera = m_renderer->cloneCamera();
  }
}

void ChartDisplay::cleanup() {
  delete m_renderer;
  m_renderer = nullptr;
  delete m_camera;
  m_camera = nullptr;
  qDebug() << "save main state";
  Conf::MainWindow::self()->save();
}

ChartDisplay::~ChartDisplay() {
  cleanup();
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
  m_chartSetChanged = true;
  emit updateViewport(m_camera, true);
  emit chartSetChanged(name);
  update();
}

QString ChartDisplay::chartSet() const {
  return ChartManager::instance()->chartSet();
}


static bool consume(bool& var) {
  const bool ret = var;
  var = false;
  return ret;
}

QSGNode* ChartDisplay::updatePaintNode(QSGNode* prev, UpdatePaintNodeData*) {

  m_renderer->setCamera(m_camera);

  if (consume(m_chartsUpdated)) {
    m_renderer->updateCharts(m_viewArea);
  }

  if (consume(m_enteringChartMode)) {
    if (m_renderer->initializeChartMode()) {
      delete m_camera;
      m_camera = m_renderer->cloneCamera();
      emit updateViewport(m_camera, true);
    }
  }

  if (consume(m_leavingChartMode)) {
    if (ChartManager::instance()->outlines().isEmpty()) {
      ChartManager::instance()->setChartSet(defaultChartSet(),
                                            m_camera->geoprojection());
    }
    if (m_renderer->finalizeChartMode()) {
      delete m_camera;
      m_camera = m_renderer->cloneCamera();
      emit updateViewport(m_camera, true);
    }
  }

  if (consume(m_chartSetChanged)) {
    m_renderer->chartSetChanged();
  }

  m_renderer->paint();

  auto node = static_cast<QSGSimpleTextureNode*>(prev);
  if (!node) {
    qDebug() << "new texture node" << m_renderer->textureId() << window()->size();
    node = new QSGSimpleTextureNode;
    node->setTexture(window()->createTextureFromId(m_renderer->textureId(),
                                                   window()->size()));
    node->setOwnsTexture(false);
    node->setFiltering(QSGTexture::Linear);
    node->setTextureCoordinatesTransform(QSGSimpleTextureNode::MirrorVertically);
  }
  node->setRect(boundingRect());
  return node;
}

void ChartDisplay::initializeGL() {
  if (m_initialized) return;

  GL::Context::instance()->initializeContext(window()->openglContext(), window());

  ChartManager::instance()->createThreads(window()->openglContext());
  TextManager::instance()->createBuffers();
  RasterSymbolManager::instance()->createSymbols();
  VectorSymbolManager::instance()->createSymbols();

  if (ChartManager::instance()->outlines().isEmpty()) {
    ChartManager::instance()->setChartSet(defaultChartSet(),
                                          m_camera->geoprojection());
  }

  m_initialized = true;
}

void ChartDisplay::resize(int) {

  if (m_size == window()->size()) return;
  m_size = window()->size();
  if (m_size.isEmpty()) return;

  const float wmm = window()->width() / dots_per_mm_x;
  const float hmm = window()->height() / dots_per_mm_y;

  try {
    m_camera->resize(wmm, hmm);
  } catch (ScaleOutOfBounds& e) {
    m_camera->setScale(e.suggestedScale());
    m_camera->resize(wmm, hmm);
  }
  qDebug() << "WxH (mm) =" << wmm << "x" << hmm;
  qDebug() << "WxH (pixels) =" << window()->width() << "x" << window()->height();
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
    update();
  }
}

void ChartDisplay::panStart(qreal x, qreal y) {
  const qreal w = window()->width();
  const qreal h = window()->height();

  m_lastPos = QPointF(2 * x / w - 1, 1 - 2 * y / h);
}

void ChartDisplay::pan(qreal x, qreal y) {
  const qreal w = window()->width();
  const qreal h = window()->height();

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

