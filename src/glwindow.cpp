#include "glwindow.h"
#include <QOpenGLExtraFunctions>
#include <QOpenGLDebugLogger>
#include <QMouseEvent>
#include <QTimer>
#include <cmath>
#include <QPainter>
#include "chartmanager.h"
#include "textmanager.h"
#include "rastersymbolmanager.h"
#include <QGuiApplication>
#include <QScreen>

GLWindow::GLWindow()
  : QOpenGLWindow(QOpenGLWindow::NoPartialUpdate)
  , m_mode(DetailMode::RestoreState())
  , m_logger(nullptr)
  , m_chartManager(ChartManager::instance())
  , m_textManager(TextManager::instance())
  , m_rasterManager(RasterSymbolManager::instance())
{
  m_timer = new QTimer(this);
  m_timer->setInterval(1000/25);
  connect(m_timer, &QTimer::timeout, this, &GLWindow::pan);

  connect(this, &GLWindow::updateViewport, m_chartManager, &ChartManager::updateCharts);
  connect(m_chartManager, &ChartManager::idle, this, &GLWindow::finalizeChartMode);
  connect(m_chartManager, &ChartManager::active, this, &GLWindow::initializeChartMode);
  connect(m_chartManager, &ChartManager::chartsUpdated, this, &GLWindow::updateCharts);

  connect(m_textManager, &TextManager::newStrings, this, [this] () {
    qDebug() << "new strings";
    emit updateViewport(m_mode->camera(), true);
    update();
  });
}


void GLWindow::saveState() {
  m_mode->saveState();
}

void GLWindow::initializeGL() {
  if (!m_logger) {
    m_logger = new QOpenGLDebugLogger(this);
    if (!m_logger->initialize()) {
      qWarning() << "OpenGL logging not available";
    }
  }

  if (!m_vao.isCreated()) {
    if (!m_vao.create()) {
      qFatal("Doh!");
    }

    m_chartManager->createThreads(context());
    m_textManager->createBuffers(context());
    m_rasterManager->createSymbols();
  }
  m_vao.bind();

  auto gl = context()->functions();
  gl->glClearColor(.4, .4, .4, 1.);

  for (Drawable* chart: m_mode->drawables()) {
    chart->initializeGL();
  }

  for (const QOpenGLDebugMessage& message: m_logger->loggedMessages()) {
    qDebug() << message;
  }
}

void GLWindow::resizeGL(int w, int h) {
  auto gl = QOpenGLContext::currentContext()->functions();
  gl->glViewport(0, 0, w, h);
  try {
    m_mode->camera()->resize(widthMM(), heightMM());
  } catch (ScaleOutOfBounds e) {
    m_mode->camera()->setScale(e.suggestedScale());
    m_mode->camera()->resize(widthMM(), heightMM());
  }
  emit updateViewport(m_mode->camera());
  qDebug() << "WxH (mm) =" << widthMM() << "x" << heightMM();
  qDebug() << "WxH (pixels) =" << width() << "x" << height();
  qDebug() << "WxH (pixels / mm) =" << QGuiApplication::primaryScreen()->physicalDotsPerInchX() / 25.4 << "x" << QGuiApplication::primaryScreen()->physicalDotsPerInchY() / 25.4;
  qDebug() << "WxH (pixels / mm) =" << static_cast<float>(width()) / widthMM() << "x" << static_cast<float>(height()) / heightMM();
}

void GLWindow::paintGL() {

  auto gl = QOpenGLContext::currentContext()->extraFunctions();

  gl->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

  for (Drawable* chart: m_mode->drawables()) {
    chart->paintGL(m_mode->camera());
  }

  for (const QOpenGLDebugMessage& message: m_logger->loggedMessages()) {
    qDebug() << message;
  }
}

static float gravity(int dx) {
  const float threshold = 10;
  const float k = 0.5;
  const float cutoff = 50;

  if (std::abs(dx) >= cutoff) return dx / std::abs(dx) * k * cutoff ;
  if (std::abs(dx) >= threshold) return k * dx;

  return 0;
}


void GLWindow::mousePressEvent(QMouseEvent* event) {
  m_timer->stop();
  m_diff *= 0;
  m_lastPos = event->pos();
  m_moveCounter = 0;
}

void GLWindow::mouseDoubleClickEvent(QMouseEvent*) {
  m_timer->stop();
  m_mode->camera()->reset();
  emit updateViewport(m_mode->camera());
  update();
}

void GLWindow::mouseReleaseEvent(QMouseEvent*) {
  if (m_diff.isNull()) return;

  m_diff.setX(gravity(m_diff.x()));
  m_diff.setY(gravity(m_diff.y()));
  m_timer->start();

}



void GLWindow::mouseMoveEvent(QMouseEvent* event) {
  if (m_timer->isActive()) return;
  m_diff = event->pos() - m_lastPos;

  if (event->buttons() == Qt::LeftButton && event->modifiers() == Qt::NoModifier) {
    if (m_moveCounter % 2 == 0) {
      pan();
      m_lastPos = event->pos();
    }
  }

  m_moveCounter = (m_moveCounter + 1) % 100000;
}

void GLWindow::wheelEvent(QWheelEvent *event) {
  if (event->angleDelta().y() > 0) {
    zoomOut();
  } else if (event->angleDelta().y() < 0) {
    zoomIn();
  }
}

void GLWindow::pan() {
  if (m_diff.isNull()) return;
  const QPointF start(2 * m_lastPos.x() / float(width()) - 1, 1 - 2 * m_lastPos.y() / float(height()));
  const QPointF amount(2 * m_diff.x() / float(width()), - 2 * m_diff.y() / float(height()));
  m_mode->camera()->pan(start, amount);
  emit updateViewport(m_mode->camera());
  update();
}

void GLWindow::compassPan(Angle bearing, float pixels) {
  if (pixels <= 0.) return;
  const Angle a = Angle::fromDegrees(90) - bearing;
  const QPointF amount(-2 * pixels * a.cos() / float(width()), -2 * pixels * a.sin() / float(height()));
  m_mode->camera()->pan(QPointF(0,0), amount);
  emit updateViewport(m_mode->camera());
  update();
}


void GLWindow::zoomIn() {
  // evenly distributed steps in log scale, div steps per decade
  const float s = m_mode->camera()->scale();
  const float s_min = m_mode->camera()->minScale();
  const float div = 10.;
  const qint32 i = qint32((log10(s/s_min)) * div + .5) - 1;
  const quint32 scale = s_min * exp10(i / div);

  if (i < 0) {
    initializeChartMode();
  }

  if (m_chartManager->isValidScale(m_mode->camera(), scale)) {
    m_mode->camera()->setScale(scale);
    emit updateViewport(m_mode->camera());
  }

  update();
}

void GLWindow::zoomOut() {
  const float s_min = m_mode->camera()->minScale();
  const float div = 10;
  const quint32 i = quint32((log10(m_mode->camera()->scale() / s_min)) * div + .5) + 1;
  const float scale = s_min * exp10(i / div);

  if (scale > m_mode->camera()->maxScale()) {
    finalizeChartMode();
  }

  if (scale < m_mode->camera()->maxScale()) {
    m_mode->camera()->setScale(scale);
    emit updateViewport(m_mode->camera());
  }

  update();
}

void GLWindow::initializeChartMode() {
  DetailMode* mode = m_mode->smallerScaleMode();
  if (mode == nullptr) {
    updateCharts();
    return;
  }
  delete m_mode;
  m_mode = mode;
  makeCurrent();
  initializeGL();
  doneCurrent();
}

void GLWindow::finalizeChartMode() {
  DetailMode* mode = m_mode->largerScaleMode();
  if (mode == nullptr) {
    updateCharts();
    return;
  }
  delete m_mode;
  m_mode = mode;
  makeCurrent();
  initializeGL();
  doneCurrent();
}

void GLWindow::northUp() {
  Angle a = m_mode->camera()->northAngle();
  m_mode->camera()->rotateEye(- a);
  emit updateViewport(m_mode->camera());
  update();
}

void GLWindow::updateCharts() {
  update();
}



GLWindow::~GLWindow() {
  makeCurrent();
  m_vao.destroy();
  doneCurrent();
  delete m_mode;
}
