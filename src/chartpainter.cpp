#include "chartpainter.h"
#include <QDebug>
#include <QOpenGLContext>
#include <QOpenGLExtraFunctions>
#include <QFile>
#include <QDir>
#include <QRegularExpression>
#include <QTextStream>
#include "s57chart.h"

ChartPainter::ChartPainter(GeoProjection* p, QObject* parent)
  : Drawable(parent)
  , m_chartData(S52::Lookup::PriorityCount)
  , m_coordBuffer(QOpenGLBuffer::VertexBuffer)
  , m_indexBuffer(QOpenGLBuffer::IndexBuffer)
{
  // TODO: osenc charts from known locations
  QDir chartDir("/home/jusirkka/.opencpn/SENC");
  QStringList files = chartDir.entryList(QStringList() << "*.S57",
                                         QDir::Files | QDir::Readable, QDir::Name);

  if (files.isEmpty()) {
    throw ChartFileError(QString("%1 is not a valid chart directory").arg(chartDir.absolutePath()));
  }
  qDebug() << files;
  QString file = files.first();
  //for (const QString& file: files) {
    try {
      auto path = chartDir.absoluteFilePath(file);
      auto chart = new S57Chart(path, p, this);
      m_charts << chart;
    } catch (ChartFileError& e) {
      qWarning() << "Chart file error:" << e.msg() << ", skipping";
    }
  //}
}

void ChartPainter::initializeGL() {
  m_program = new QOpenGLShaderProgram(this);

  struct Source {
    QOpenGLShader::ShaderTypeBit stype;
    QString fname;
  };

  const QVector<Source> sources{
    {QOpenGLShader::Vertex, ":/shaders/chartpainter.vert"},
    {QOpenGLShader::Fragment, ":/shaders/chartpainter.frag"},
  };

  for (const Source& s: sources) {
    if (!m_program->addCacheableShaderFromSourceFile(s.stype, s.fname)) {
      qFatal("Failed to compile %s: %s", s.fname.toUtf8().data(), m_program->log().toUtf8().data());
    }
  }

  if (!m_program->link()) {
    qFatal("Failed to link the chartpainter program: %s", m_program->log().toUtf8().data());
  }

  QVector<GLfloat> vertices;
  QVector<GLuint> indices;

  GLsizei vertexOffset = 0;
  GLsizei elementOffset = 0;
  for (S57Chart* chart: m_charts) {
    vertices.append(chart->vertices());
    indices.append(chart->indices());
    for (int i = 0; i < S52::Lookup::PriorityCount; i++) {
      ChartData d;
      d.elementOffset = elementOffset;
      d.vertexOffset = vertexOffset;
      d.lineData = chart->lines(i);
      d.triangleData = chart->triangles(i);
      m_chartData[i].append(d);
    }
    elementOffset += chart->indices().size() * sizeof(GLuint);
    vertexOffset += chart->vertices().size() * sizeof(GLfloat);
  }

  // buffers
  m_coordBuffer.create();
  m_coordBuffer.bind();
  m_coordBuffer.setUsagePattern(QOpenGLBuffer::DynamicDraw);
  GLsizei dataLen = vertices.size() * sizeof(GLfloat);
  m_coordBuffer.allocate(dataLen);
  m_coordBuffer.write(0, vertices.constData(), dataLen);

  m_indexBuffer.create();
  m_indexBuffer.bind();
  m_indexBuffer.setUsagePattern(QOpenGLBuffer::DynamicDraw);
  GLsizei elemLen = indices.size() * sizeof(GLuint);
  m_indexBuffer.allocate(elemLen);
  m_indexBuffer.write(0, indices.constData(), elemLen);

  // locations
  m_program->bind();
  m_locations.base_color = m_program->uniformLocation("base_color");
  m_locations.m_pvm = m_program->uniformLocation("m_pvm");

  auto gl = QOpenGLContext::currentContext()->functions();
  gl->glDisable(GL_DEPTH_TEST);
  gl->glEnable(GL_STENCIL_TEST);
  gl->glDisable(GL_CULL_FACE);
  gl->glStencilFuncSeparate(GL_FRONT, GL_EQUAL, 0, 0xff);
  gl->glStencilOpSeparate(GL_FRONT, GL_KEEP, GL_KEEP, GL_INCR);
}


void ChartPainter::paintGL(const Camera* cam) {
  auto gl = QOpenGLContext::currentContext()->extraFunctions();

  m_program->bind();

  m_program->enableAttributeArray(0);
  m_coordBuffer.bind();
  m_indexBuffer.bind();

  QVector<QMatrix4x4> modelTransforms;
  for (const S57Chart* chart: m_charts) {
    QVector2D p = cam->geoprojection()->fromWGS84(chart->geoProjection()->reference());
    QMatrix4x4 tr;
    tr.translate(p.x(), p.y());
    modelTransforms.append(tr);
  }

  for (int i = S52::Lookup::PriorityCount - 1; i >= 0; i--) {
    for (int j = 0; j < m_chartData[i].size(); j++) {
      const ChartData d = m_chartData[i][j];
      const QMatrix4x4 tr = modelTransforms[j];
      m_program->setUniformValue(m_locations.m_pvm,
                                 cam->projection() * cam->view() * tr);
      for (const S57::PaintData& item: d.lineData) {
        m_program->setUniformValue(m_locations.base_color, item.color);
        m_program->setAttributeBuffer(0, GL_FLOAT, d.vertexOffset + item.vertexOffset, 2, 0);
        for (const S57::ElementData& e: item.elements) {
          gl->glDrawElements(e.mode, e.elementCount, GL_UNSIGNED_INT,
                             (const void*) (d.elementOffset + e.elementOffset));
        }
      }
      for (const S57::PaintData& item: d.triangleData) {
        m_program->setUniformValue(m_locations.base_color, item.color);
        m_program->setAttributeBuffer(0, GL_FLOAT, d.vertexOffset + item.vertexOffset, 2, 0);
        for (const S57::ElementData& e: item.elements) {
          gl->glDrawElements(e.mode, e.elementCount, GL_UNSIGNED_INT,
                             (const void*) (d.elementOffset + e.elementOffset));
        }
      }
    }
  }
}
