#include "outliner.h"
#include <QDebug>
#include <QOpenGLContext>
#include <QOpenGLExtraFunctions>
#include <QFile>
#include <QDir>
#include <QRegularExpression>
#include <QTextStream>
#include "chartmanager.h"
#include "s57chart.h"

Outliner::Outliner(QObject* parent)
  : Drawable(parent)
  , m_cornerBuffer(QOpenGLBuffer::VertexBuffer)
  , m_program(nullptr)
  , m_manager(ChartManager::instance())
{}

void Outliner::initializeGL() {
  if (m_program == nullptr) {
    m_program = new QOpenGLShaderProgram(this);

    struct Source {
      QOpenGLShader::ShaderTypeBit stype;
      QString fname;
    };
    const QVector<Source> sources{
      {QOpenGLShader::Vertex, ":/shaders/outliner.vert"},
      {QOpenGLShader::Fragment, ":/shaders/outliner.frag"},
    };

    for (const Source& s: sources) {
      if (!m_program->addShaderFromSourceFile(s.stype, s.fname)) {
        qFatal("Failed to compile %s: %s", s.fname.toUtf8().data(), m_program->log().toUtf8().data());
      }
    }

    if (!m_program->link()) {
      qFatal("Failed to link the outliner program: %s", m_program->log().toUtf8().data());
    }

    m_cornerBuffer.create();
    m_cornerBuffer.setUsagePattern(QOpenGLBuffer::DynamicDraw);

    // locations
    m_program->bind();
    m_locations.base_color = m_program->uniformLocation("base_color");
    m_locations.m_pv = m_program->uniformLocation("m_pv");
    m_locations.center = m_program->uniformLocation("center");
    m_locations.angle = m_program->uniformLocation("angle");
    m_locations.nump = m_program->uniformLocation("nump");

  }

  updateBuffers();
}

void Outliner::updateCharts(const Camera* /*cam*/, const QRectF& /*viewArea*/) {
  // noop
}

void Outliner::paintGL(const Camera* cam) {

  m_cornerBuffer.bind();

  m_program->bind();
  m_program->enableAttributeArray(0);
  m_program->setAttributeBuffer(0, GL_FLOAT, 0, 4, 0);

  auto f = QOpenGLContext::currentContext()->extraFunctions();
  f->glVertexAttribDivisor(0, 1);


  f->glEnable(GL_DEPTH_TEST);
  f->glDisable(GL_BLEND);
  f->glDisable(GL_STENCIL_TEST);
  f->glEnable(GL_CULL_FACE);
  f->glFrontFace(GL_CW);
  f->glCullFace(GL_BACK);

  // uniforms
  m_program->setUniformValue(m_locations.m_pv, cam->projection() * cam->view());
  GLfloat angle = qMax(1.e-5, GLfloat(cam->scale()) / cam->maxScale() * 0.005);
  m_program->setUniformValue(m_locations.angle, angle);
  m_program->setUniformValue(m_locations.nump, 10);
  m_program->setUniformValue(m_locations.base_color, QColor("#ff0000"));

  f->glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 2 * (4 * 10 + 1), m_instances);

  f->glVertexAttribDivisor(0, 0);

}


void Outliner::updateBuffers() {

  m_cornerBuffer.bind();


  m_instances = m_manager->outlines().size() / 8;

  GL::VertexVector corners;

  const GL::VertexVector &d = m_manager->outlines();

  for (int i = 0; i < m_instances; ++i) {
    const int first = 8 * i;
    // sw, ne
    corners << d[first + 2] << d[first + 3] << d[first + 6] << d[first + 7];
  }

  GLsizei dataLen = corners.size() * sizeof(GLfloat);
  m_cornerBuffer.allocate(dataLen);
  m_cornerBuffer.write(0, corners.constData(), dataLen);

  qDebug() << "number of rectangles =" << m_instances;
}
