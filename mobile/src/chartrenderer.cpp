#include "chartrenderer.h"
#include <QQuickWindow>
#include "camera.h"
#include "detailmode.h"
#include <QOpenGLDebugLogger>

ChartRenderer::ChartRenderer(QQuickWindow *window)
  : m_mode(DetailMode::RestoreState())
  , m_window(window)
  , m_logger(nullptr)
  , m_fbo(nullptr)
{
  connect(window, &QQuickWindow::sceneGraphInitialized, this, &ChartRenderer::initializeGL, Qt::DirectConnection);
}

void ChartRenderer::initializeGL() {
  if (!m_logger) {
    m_logger = new QOpenGLDebugLogger(this);
    if (!m_logger->initialize()) {
      qWarning() << "OpenGL logging not available";
    }
  }

  if (!m_fbo) {
    QOpenGLFramebufferObjectFormat fmt;
    fmt.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
    m_fbo = new QOpenGLFramebufferObject(m_window->size(), fmt);
  }

  for (Drawable* drawable: m_mode->drawables()) {
    drawable->initializeGL();
  }
  for (const QOpenGLDebugMessage& message: m_logger->loggedMessages()) {
    qDebug() << "Init:" << message;
  }
}

void ChartRenderer::setCamera(const Camera *cam) {
  m_mode->camera()->update(cam);
}

Camera* ChartRenderer::cloneCamera() const {
  return m_mode->cloneCamera();
}

void ChartRenderer::updateCharts(const QRectF& viewArea) {
  for (Drawable* d: m_mode->drawables()) {
    d->updateCharts(m_mode->camera(), viewArea);
  }
}

bool ChartRenderer::initializeChartMode() {
  qDebug() << "Initialize chart mode";
  DetailMode* mode = m_mode->smallerScaleMode();
  if (mode == nullptr) return false;
  delete m_mode;
  m_mode = mode;
  initializeGL();
  return true;
}

bool ChartRenderer::finalizeChartMode() {
  qDebug() << "Finalize chart mode";
  DetailMode* mode = m_mode->largerScaleMode();
  if (mode == nullptr) return false;
  delete m_mode;
  m_mode = mode;
  initializeGL();
  return true;
}

void ChartRenderer::chartSetChanged() {
  initializeGL();
}

void ChartRenderer::paint() {

  m_fbo->bind();

  auto f = QOpenGLContext::currentContext()->functions();

  f->glClearStencil(0x00);
  f->glClearDepthf(1.);
  f->glClearColor(.4, .4, .4, 1.);
  f->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
  f->glViewport(0, 0, m_window->width(), m_window->height());

  for (Drawable* drawable: m_mode->drawables()) {
    drawable->paintGL(m_mode->camera());
  }
  for (const QOpenGLDebugMessage& message: m_logger->loggedMessages()) {
    qDebug() << "Paint:" << message;
  }
  m_window->openglContext()->functions()->glFlush();
  m_fbo->bindDefault();

  // Generally useful for when mixing with raw OpenGL.
  // m_window->resetOpenGLState();
}

GLuint ChartRenderer::textureId() const {
  return m_fbo->texture();
}

ChartRenderer::~ChartRenderer() {
  qDebug() << "save mode state";
  m_mode->saveState();
  delete m_mode;
}
