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

  auto gl = QOpenGLContext::currentContext()->functions();
  gl->glEnable(GL_DEPTH_TEST);
  gl->glDisable(GL_STENCIL_TEST);
  gl->glDisable(GL_CULL_FACE);
}


void ChartPainter::paintGL(const Camera* cam) {


  for (S57Chart* chart: m_manager->charts()) {
    chart->setTransform(cam);
  }

  // draw with non-discarding progs nearest objects (highest priority) first
  for (int i = S52::Lookup::PriorityCount - 1; i >= 0; i--) {
    m_areaShader->initializePaint();
    for (S57Chart* chart: m_manager->charts()) {
      chart->drawAreas(i);
    }
    m_solidShader->initializePaint();
    m_solidShader->setScreen(cam);
    for (S57Chart* chart: m_manager->charts()) {
      chart->drawSolidLines(i);
    }
  }

  // draw with discarding progs in any order
  m_dashedShader->initializePaint();
  m_dashedShader->setScreen(cam);

  for (S57Chart* chart: m_manager->charts()) {
    chart->drawDashedLines();
  }
}

