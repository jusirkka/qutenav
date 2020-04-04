#include "outliner.h"
#include <QDebug>
#include <QOpenGLContext>
#include <QOpenGLExtraFunctions>
#include <QFile>
#include <QDir>
#include <QRegularExpression>
#include <QTextStream>
#include "s57chart.h"

Outliner::Outliner()
{

  // TODO: osenc charts from known locations
  QDir chartDir("/home/jusirkka/.opencpn/SENC");
  QStringList files = chartDir.entryList(QStringList() << "*.S57",
                                         QDir::Files | QDir::Readable, QDir::Name);

  if (files.isEmpty()) {
    throw ChartFileError(QString("%1 is not a valid chart directory").arg(chartDir.absolutePath()));
  }
  qDebug() << files;

  for (const QString& file: files) {
    auto path = chartDir.absoluteFilePath(file);
    S57Chart chart(path);
    try {
    } catch (ChartFileError& e) {
      qWarning() << "Chart file error:" << e.msg() << ", skipping";
      continue;
    }
    m_outlines.append(chart.extent().eightFloater());
  }

  qDebug() << "number of rectangles =" << m_outlines.size() / 8;

  m_Params.color = QColor("#ff0000");
  m_Params.length = m_outlines.size() / 2;
}

void Outliner::initializeGL(QOpenGLWidget *context) {
  m_program = new QOpenGLShaderProgram(context);

  struct Source {
    QOpenGLShader::ShaderTypeBit stype;
    QString fname;
  };
  const QVector<Source> sources{
    {QOpenGLShader::Vertex, ":/shaders/simple.vert"},
    {QOpenGLShader::TessellationControl, ":/shaders/outliner.tesc"},
    {QOpenGLShader::TessellationEvaluation, ":/shaders/outliner.tese"},
    {QOpenGLShader::Fragment, ":/shaders/outliner.frag"},
  };

  for (const Source& s: sources) {
    QFile file(s.fname);
    if (!file.open(QIODevice::ReadOnly)) {
      qFatal("Failed to open %s for reading", s.fname.toUtf8().data());
    }
    QString src = file.readAll();
    src = src.replace("#version 320 es", "#version 450 core");
    if (!m_program->addCacheableShaderFromSourceCode(s.stype, src)) {
      qFatal("Failed to compile %s: %s", s.fname.toUtf8().data(), m_program->log().toUtf8().data());
    }
  }

  if (!m_program->link()) {
    qFatal("Failed to link the outliner program: %s", m_program->log().toUtf8().data());
  }

  m_coordBuffer.create();
  m_coordBuffer.bind();
  m_coordBuffer.setUsagePattern(QOpenGLBuffer::StaticDraw);
  GLsizei dataLen = m_outlines.size() * sizeof(GLfloat);
  m_coordBuffer.allocate(dataLen);
  m_coordBuffer.write(0, m_outlines.constData(), dataLen);
  // not needed anymore
  m_outlines.clear();

  // locations
  m_program->bind();
  m_locations.point = m_program->attributeLocation("point");
  m_locations.base_color = m_program->uniformLocation("base_color");
  m_locations.m_pv = m_program->uniformLocation("m_pv");
  m_locations.eye = m_program->uniformLocation("eye");

}

void Outliner::paintGL(const Camera* cam) {
  auto gl = QOpenGLContext::currentContext()->extraFunctions();

  m_program->bind();

  // data buffers
  m_program->enableAttributeArray(m_locations.point);
  m_coordBuffer.bind();
  m_program->setAttributeBuffer(m_locations.point, GL_FLOAT, 0, 2, 0);

  // uniforms
  const QVector3D eye = cam->view().inverted().column(3).toVector3D();
  m_program->setUniformValue(m_locations.eye, eye);
  m_program->setUniformValue(m_locations.m_pv, cam->projection() * cam->view());
  m_program->setUniformValue(m_locations.base_color, m_Params.color);

  gl->glPatchParameteri(GL_PATCH_VERTICES, 4);

  gl->glDrawArrays(GL_PATCHES, 0, m_Params.length);
}

