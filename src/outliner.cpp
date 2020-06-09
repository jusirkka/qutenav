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
  , m_manager(ChartManager::instance())
{}

void Outliner::initializeGL() {
  m_program = new QOpenGLShaderProgram(this);

  struct Source {
    QOpenGLShader::ShaderTypeBit stype;
    QString fname;
  };
  const QVector<Source> sources{
    {QOpenGLShader::Vertex, ":/shaders/outliner.vert"},
    {QOpenGLShader::Geometry, ":/shaders/outliner.geom"},
    {QOpenGLShader::Fragment, ":/shaders/outliner.frag"},
  };

  for (const Source& s: sources) {
    if (!m_program->addCacheableShaderFromSourceFile(s.stype, s.fname)) {
      qFatal("Failed to compile %s: %s", s.fname.toUtf8().data(), m_program->log().toUtf8().data());
    }
  }

  if (!m_program->link()) {
    qFatal("Failed to link the outliner program: %s", m_program->log().toUtf8().data());
  }

  updateCharts();

  // locations
  m_program->bind();
  m_locations.base_color = m_program->uniformLocation("base_color");
  m_locations.m_pv = m_program->uniformLocation("m_pv");
  m_locations.center = m_program->uniformLocation("center");
  m_locations.angle = m_program->uniformLocation("angle");

  auto gl = QOpenGLContext::currentContext()->functions();
  gl->glEnable(GL_DEPTH_TEST);
  gl->glEnable(GL_STENCIL_TEST);
  gl->glEnable(GL_CULL_FACE);
  gl->glFrontFace(GL_CCW);
  gl->glCullFace(GL_BACK);
  gl->glStencilFuncSeparate(GL_FRONT, GL_EQUAL, 0, 0xff);
  gl->glStencilOpSeparate(GL_FRONT, GL_KEEP, GL_KEEP, GL_INCR);

}

void Outliner::paintGL(const Camera* cam) {
  auto gl = QOpenGLContext::currentContext()->extraFunctions();

  m_program->bind();

  // data buffers
  m_program->enableAttributeArray(0);
  m_coordBuffer.bind();

  // uniforms
  m_program->setUniformValue(m_locations.m_pv, cam->projection() * cam->view());
  GLfloat angle = qMax(1.e-6, GLfloat(cam->scale()) / cam->maxScale() * 0.005);
  m_program->setUniformValue(m_locations.angle, angle);

  for (const Rectangle& outline: m_outlines) {
    m_program->setUniformValue(m_locations.base_color, outline.color);
    m_program->setUniformValue(m_locations.center, outline.center);
    m_program->setAttributeBuffer(0, GL_FLOAT, outline.offset, 3, 0);
    gl->glDrawArrays(GL_LINE_LOOP, 0, 4);
  }
}


void Outliner::updateBuffers() {
  if (!m_coordBuffer.isCreated()) {
    m_coordBuffer.create();
    m_coordBuffer.setUsagePattern(QOpenGLBuffer::DynamicDraw);
  }
  m_coordBuffer.bind();

  const int cnt = m_manager->outlines().size() / 8;
  DataVector outlines;
  for (int i = 0; i < cnt; ++i) {
    m_outlines.append(Rectangle(m_manager->outlines(), i));
    outlines.append(m_outlines.last().outline);
  }

  GLsizei dataLen = outlines.size() * sizeof(GLfloat);
  m_coordBuffer.allocate(dataLen);
  m_coordBuffer.write(0, outlines.constData(), dataLen);

  qDebug() << "number of rectangles =" << cnt;
}

Outliner::Rectangle::Rectangle(const DataVector &d, int rectIndex)
  : color("#ff0000"),
    offset(rectIndex * 12 * sizeof(GLfloat))
{

  int first = rectIndex * 8;
  QVector3D sum(0., 0., 0.);
  for (int i = 0; i < 4; i++) {
    const float lng = d[first + 2 * i] * M_PI / 180;
    const float lat = d[first + 2 * i + 1] * M_PI / 180;
    QVector3D p(cos(lng) * cos(lat),
                sin(lng) * cos(lat),
                sin(lat));
    outline.append(p.x());
    outline.append(p.y());
    outline.append(p.z());
    sum += p;
  }
  center = sum.normalized();
}

