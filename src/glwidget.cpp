#include "glwidget.h"
#include <QOpenGLFunctions>
#include <QOpenGLDebugLogger>
#include <QMouseEvent>
#include <QTimer>
#include <cmath>
#include <QPainter>
#include "chartmanager.h"

GLWidget::GLWidget(QWidget* parent)
  : QOpenGLWidget(parent)
  , m_mode(DetailMode::RestoreState())
  , m_logger(nullptr)
  , m_manager(ChartManager::instance())
{
  m_timer = new QTimer(this);
  m_timer->setInterval(1000/25);
  connect(m_timer, &QTimer::timeout, this, &GLWidget::pan);

  connect(this, &GLWidget::updateViewport, m_manager, &ChartManager::updateCharts);
  connect(m_manager, &ChartManager::idle, this, &GLWidget::finalizeChartMode);
  connect(m_manager, &ChartManager::active, this, &GLWidget::initializeChartMode);
  connect(m_manager, &ChartManager::chartsUpdated, this, &GLWidget::updateCharts);
}


void GLWidget::saveState() {
  m_mode->saveState();
}

void GLWidget::initializeGL() {
  if (!m_logger) {
    m_logger = new QOpenGLDebugLogger(this);
    if (!m_logger->initialize()) {
      qWarning() << "OpenGL logging not available";
    }
  }

  m_manager->createThreads(context());

  auto gl = context()->functions();
  gl->glClearColor(.4, .4, .4, 1.);

  if (!m_vao.isCreated()) {
    if (!m_vao.create()) {
      qFatal("Doh!");
    }
  }
  m_vao.bind();

  for (Drawable* chart: m_mode->drawables()) {
    chart->initializeGL();
  }

  for (const QOpenGLDebugMessage& message: m_logger->loggedMessages()) {
    qDebug() << message;
  }
}

void GLWidget::resizeGL(int w, int h) {
  auto gl = QOpenGLContext::currentContext()->functions();
  gl->glViewport(0, 0, w, h);
  try {
    m_mode->camera()->resize(widthMM(), heightMM());
  } catch (ScaleOutOfBounds e) {
    m_mode->camera()->setScale(e.suggestedScale());
    m_mode->camera()->resize(widthMM(), heightMM());
  }
  emit updateViewport(m_mode->camera());
}

void GLWidget::paintGL() {

  auto gl = QOpenGLContext::currentContext()->functions();
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


void GLWidget::mousePressEvent(QMouseEvent* event) {
  m_timer->stop();
  m_diff *= 0;
  m_lastPos = event->pos();
  m_moveCounter = 0;
}

void GLWidget::mouseDoubleClickEvent(QMouseEvent*) {
  m_timer->stop();
  m_mode->camera()->reset();
  emit updateViewport(m_mode->camera());
  update();
}

void GLWidget::mouseReleaseEvent(QMouseEvent*) {
  if (m_diff.isNull()) return;

  m_diff.setX(gravity(m_diff.x()));
  m_diff.setY(gravity(m_diff.y()));
  m_timer->start();

}



void GLWidget::mouseMoveEvent(QMouseEvent* event) {
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

void GLWidget::wheelEvent(QWheelEvent *event) {
  if (event->angleDelta().y() > 0) {
    zoomOut();
  } else if (event->angleDelta().y() < 0) {
    zoomIn();
  }
}

void GLWidget::pan() {
  if (m_diff.isNull()) return;
  const QPointF start(2 * m_lastPos.x() / float(width()) - 1, 1 - 2 * m_lastPos.y() / float(height()));
  const QPointF amount(2 * m_diff.x() / float(width()), - 2 * m_diff.y() / float(height()));
  m_mode->camera()->pan(start, amount);
  emit updateViewport(m_mode->camera());
  update();
}

void GLWidget::compassPan(Angle bearing, float pixels) {
  if (pixels <= 0.) return;
  const Angle a = Angle::fromDegrees(90) - bearing;
  const QPointF amount(-2 * pixels * a.cos() / float(width()), -2 * pixels * a.sin() / float(height()));
  m_mode->camera()->pan(QPointF(0,0), amount);
  emit updateViewport(m_mode->camera());
  update();
}


void GLWidget::zoomIn() {
  // evenly distributed steps in log scale, div steps per decade
  const float s = m_mode->camera()->scale();
  const float s_min = m_mode->camera()->minScale();
  const float div = 10.;
  const qint32 i = qint32((log10(s/s_min)) * div + .5) - 1;
  const quint32 scale = s_min * exp10(i / div);

  if (i < 0) {
    initializeChartMode();
  }

  if (m_manager->isValidScale(m_mode->camera(), scale)) {
    m_mode->camera()->setScale(scale);
    emit updateViewport(m_mode->camera());
  }

  update();
}

void GLWidget::zoomOut() {
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

void GLWidget::initializeChartMode() {
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

void GLWidget::finalizeChartMode() {
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

void GLWidget::northUp() {
  Angle a = m_mode->camera()->northAngle();
  m_mode->camera()->rotateEye(- a);
  emit updateViewport(m_mode->camera());
  update();
}

void GLWidget::updateCharts() {
  update();
}



GLWidget::~GLWidget() {
  makeCurrent();
  m_vao.destroy();
  doneCurrent();
  delete m_mode;
}
