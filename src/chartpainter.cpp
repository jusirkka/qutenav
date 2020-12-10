#include "chartpainter.h"
#include <QDebug>
#include <QOpenGLContext>
#include <QOpenGLExtraFunctions>
#include "s57chart.h"
#include "chartmanager.h"
#include <QGuiApplication>
#include <QScreen>
#include "shader.h"
#include <QOpenGLFramebufferObject>

ChartPainter::ChartPainter(QObject* parent)
  : Drawable(parent)
  , m_manager(ChartManager::instance())
{}

void ChartPainter::initializeGL() {

  m_areaShader = GL::AreaShader::instance();
  m_solidShader = GL::SolidLineShader::instance();
  m_dashedShader = GL::DashedLineShader::instance();
  m_textShader = GL::TextShader::instance();
  m_rasterShader = GL::RasterSymbolShader::instance();

  auto gl = QOpenGLContext::currentContext()->functions();
  gl->glEnable(GL_DEPTH_TEST);
  gl->glDisable(GL_STENCIL_TEST);
  gl->glDisable(GL_CULL_FACE);
  gl->glDisable(GL_BLEND);
  gl->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}


void ChartPainter::paintGL(const Camera* cam) {

  auto gl = QOpenGLContext::currentContext()->functions();

  // draw opaque objects nearest objects (highest priority) first
  for (int i = S52::Lookup::PriorityCount - 1; i >= 0; i--) {
    m_solidShader->initializePaint();
    for (S57Chart* chart: m_manager->charts()) {
      chart->drawSolidLines(cam, i);
    }
    m_areaShader->initializePaint();
    for (S57Chart* chart: m_manager->charts()) {
      chart->drawAreas(cam, i);
    }
  }

  gl->glEnable(GL_BLEND);

  // draw translucent objects farthest first
  for (int i = 0; i < S52::Lookup::PriorityCount; i++) {
    m_dashedShader->initializePaint();
    for (S57Chart* chart: m_manager->charts()) {
      chart->drawDashedLines(cam, i);
    }
    m_rasterShader->initializePaint();
    for (S57Chart* chart: m_manager->charts()) {
      chart->drawRasterSymbols(cam, i);
    }
    m_textShader->initializePaint();
    for (S57Chart* chart: m_manager->charts()) {
      chart->drawText(cam, i);
    }
  }

  // draw stencilled objects
  m_rasterShader->initializePaint();
  for (S57Chart* chart: m_manager->charts()) {
    chart->drawPatterns(cam);
  }

  gl->glDisable(GL_BLEND);



}

