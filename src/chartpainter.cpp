#include "chartpainter.h"
#include <QDebug>
#include <QOpenGLContext>
#include <QOpenGLExtraFunctions>
#include "s57chart.h"
#include "chartmanager.h"
#include <QGuiApplication>
#include <QScreen>
#include "shader.h"

ChartPainter::ChartPainter(QObject* parent)
  : Drawable(parent)
  , m_manager(ChartManager::instance())
{}

void ChartPainter::initializeGL() {

  m_areaShader = GL::AreaShader::instance();
  m_solidShader = GL::SolidLineShader::instance();
  m_dashedShader = GL::DashedLineShader::instance();
  m_textShader = GL::TextShader::instance();

  auto gl = QOpenGLContext::currentContext()->functions();
  gl->glEnable(GL_DEPTH_TEST);
  gl->glDisable(GL_STENCIL_TEST);
  gl->glDisable(GL_CULL_FACE);
  gl->glEnable(GL_BLEND);
  gl->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}


void ChartPainter::paintGL(const Camera* cam) {

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

  // draw translucent objects
  m_dashedShader->initializePaint();
  for (S57Chart* chart: m_manager->charts()) {
    chart->drawDashedLines(cam);
  }
  m_textShader->initializePaint();
  for (S57Chart* chart: m_manager->charts()) {
    chart->drawText(cam);
  }

}

