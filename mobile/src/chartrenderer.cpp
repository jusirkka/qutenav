#include "chartrenderer.h"
#include <QQuickWindow>
#include "camera.h"
#include "detailmode.h"
#include "chartdisplay.h"
#include <QOpenGLFramebufferObjectFormat>

ChartRenderer::ChartRenderer(QQuickWindow *window)
  : m_mode(DetailMode::RestoreState())
  , m_window(window)
{
  m_vao.create();
  initializeGL();
}

void ChartRenderer::initializeGL() {

  m_vao.bind();

  for (Drawable* drawable: m_mode->drawables()) {
    drawable->initializeGL();
  }
}


bool ChartRenderer::initializeChartMode() {
  DetailMode* mode = m_mode->smallerScaleMode();
  if (mode == nullptr) return false;
  qDebug() << "Initialize chart mode";
  delete m_mode;
  m_mode = mode;
  initializeGL();
  return true;
}

bool ChartRenderer::finalizeChartMode() {
  DetailMode* mode = m_mode->largerScaleMode();
  if (mode == nullptr) return false;
  qDebug() << "Finalize chart mode";
  delete m_mode;
  m_mode = mode;
  initializeGL();
  return true;
}


void ChartRenderer::render() {

  m_vao.bind();
  auto f = QOpenGLContext::currentContext()->functions();

  f->glClearStencil(0x00);
  f->glClearDepthf(1.);
  f->glClearColor(.4, .4, .4, 1.);
  f->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

  for (Drawable* drawable: m_mode->drawables()) {
    drawable->paintGL(m_mode->camera());
  }

  m_window->resetOpenGLState();
}

QOpenGLFramebufferObject* ChartRenderer::createFramebufferObject(const QSize& size) {
  qDebug() << "ChartRenderer::createFramebufferObject";
  QOpenGLFramebufferObjectFormat fmt;
  fmt.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
  return new QOpenGLFramebufferObject(size, fmt);
}

ChartRenderer::~ChartRenderer() {
  qDebug() << "ChartRenderer::~ChartRenderer";
  m_mode->saveState();
  delete m_mode;
}

void ChartRenderer::synchronize(QQuickFramebufferObject *parent) {

  m_vao.bind();

  auto item = qobject_cast<ChartDisplay*>(parent);

  m_mode->camera()->update(item->camera());

  if (item->consume(ChartDisplay::ChartsUpdated)) {
    for (Drawable* d: m_mode->drawables()) {
      d->updateCharts(m_mode->camera(), item->viewArea());
    }
  }

  if (item->consume(ChartDisplay::EnteringChartMode)) {
    if (initializeChartMode()) {
      item->setCamera(m_mode->cloneCamera());
    }
  }

  if (item->consume(ChartDisplay::LeavingChartMode)) {
    item->checkChartSet();
    if (finalizeChartMode()) {
      item->setCamera(m_mode->cloneCamera());
    }
  }

  if (item->consume(ChartDisplay::ChartSetChanged)) {
    initializeGL();
  }
}

