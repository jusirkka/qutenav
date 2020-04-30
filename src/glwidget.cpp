#include "glwidget.h"
#include <QOpenGLFunctions>
#include <QOpenGLDebugLogger>
#include <QMouseEvent>
#include <QTimer>
#include <cmath>
#include <QPainter>

GLWidget::GLWidget(QWidget* parent)
  : QOpenGLWidget(parent)
  , m_mode(DetailMode::RestoreState())
  , m_logger(nullptr)
{
  m_timer = new QTimer(this);
  m_timer->setInterval(1000/25);
  connect(m_timer, &QTimer::timeout, this, &GLWidget::pan);
}


void GLWidget::saveState() {
  m_mode->saveState(widthMM(), heightMM());
}

void GLWidget::initializeGL() {
  m_logger = new QOpenGLDebugLogger(this);
  if (!m_logger->initialize()) {
    qWarning() << "OpenGL logging not available";
  }

  auto gl = QOpenGLContext::currentContext()->functions();
  gl->glClearColor(.0, .0, .2, 1.);
  gl->glEnable(GL_DEPTH_TEST);
  gl->glEnable(GL_STENCIL_TEST);
  gl->glEnable(GL_CULL_FACE);
  gl->glFrontFace(GL_CCW);
  gl->glCullFace(GL_BACK);



  if (!m_vao.create()) {
    qFatal("Doh!");
  }
  m_vao.bind();

  for (Drawable* chart: m_mode->drawables()) {
    chart->initializeGL(this);
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
}

void GLWidget::paintGL() {

  auto gl = QOpenGLContext::currentContext()->functions();
  gl->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

  gl->glStencilFuncSeparate(GL_FRONT, GL_EQUAL, 0, 0xff);
  gl->glStencilOpSeparate(GL_FRONT, GL_KEEP, GL_KEEP, GL_INCR);

  for (Drawable* chart: m_mode->drawables()) {
    chart->paintGL(m_mode->camera());
  }
  for (const QOpenGLDebugMessage& message: m_logger->loggedMessages()) {
    qDebug() << message;
  }
}

static float gravity(int dx) {
  const float threshold = 2;
  const float k = 1.0;
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
  const QVector2D start(2 * m_lastPos.x() / float(width()) - 1, 1 - 2 * m_lastPos.y() / float(height()));
  const QVector2D amount(2 * m_diff.x() / float(width()), - 2 * m_diff.y() / float(height()));
  m_mode->camera()->pan(start, amount);
  update();
}

void GLWidget::compassPan(Angle bearing, float pixels) {
  if (pixels <= 0.) return;
  const Angle a = Angle::fromDegrees(90) - bearing;
  const QVector2D amount(-2 * pixels * a.cos() / float(width()), -2 * pixels * a.sin() / float(height()));
  m_mode->camera()->pan(QVector2D(0,0), amount);
  update();
}


void GLWidget::zoomIn() {
  // evenly distributed steps in log scale, div steps per decade
  const float s = m_mode->camera()->scale();
  const float s_min = m_mode->camera()->minScale();
  const float div = 10;
  const qint32 i = qint32((log10(s/s_min)) * div + .5) - 1;
  if (i < 0) {
    DetailMode* mode = m_mode->smallerScaleMode(widthMM(), heightMM());
    if (mode == nullptr) return;
    delete m_mode;
    m_mode = mode;
  }
  m_mode->camera()->setScale(s_min * exp10(i / div));
  update();
}

void GLWidget::zoomOut() {
  const float s_min = m_mode->camera()->minScale();
  const float div = 10;
  const quint32 i = quint32((log10(m_mode->camera()->scale()/s_min)) * div + .5) + 1;
  const float s = s_min * exp10(i / div);
  if (s > m_mode->camera()->maxScale()) {
    DetailMode* mode = m_mode->largerScaleMode(widthMM(), heightMM());
    if (mode == nullptr) return;
    delete m_mode;
    m_mode = mode;
  }
  m_mode->camera()->setScale(s);
  update();
}


void GLWidget::northUp() {
  Angle a = m_mode->camera()->northAngle();
  m_mode->camera()->rotateEye(- a);
  update();
}


GLWidget::~GLWidget() {
  makeCurrent();
  m_vao.destroy();
  doneCurrent();
  delete m_mode;
}
