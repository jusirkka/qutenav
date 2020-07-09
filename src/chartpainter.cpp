#include "chartpainter.h"
#include <QDebug>
#include <QOpenGLContext>
#include <QOpenGLExtraFunctions>
#include "s57chart.h"
#include "chartmanager.h"
#include <QGuiApplication>
#include <QScreen>

ChartPainter::ChartPainter(QObject* parent)
  : Drawable(parent)
  , m_chartData(S52::Lookup::PriorityCount)
  , m_coordBuffer(QOpenGLBuffer::VertexBuffer)
  , m_indexBuffer(QOpenGLBuffer::IndexBuffer)
  , m_manager(ChartManager::instance())
{}

void ChartPainter::initializeGL() {
  m_program = new QOpenGLShaderProgram(this);
  m_lineProg = new QOpenGLShaderProgram(this);

  struct Source {
    QOpenGLShader::ShaderTypeBit stype;
    QString fname;
  };

  struct Prog {
    QOpenGLShaderProgram* prog;
    QVector<Source> sources;
  };

  const QVector<Prog> progs{
    {m_program, {
        {QOpenGLShader::Vertex, ":/shaders/chartpainter.vert"},
        {QOpenGLShader::Fragment, ":/shaders/chartpainter.frag"}
      }
    },
    {m_lineProg, {
        {QOpenGLShader::Vertex, ":/shaders/chartpainter.vert"},
        {QOpenGLShader::Geometry, ":/shaders/chartpainter-lines.geom"},
        {QOpenGLShader::Fragment, ":/shaders/chartpainter-lines.frag"}
      }
    }
  };


  for (const Prog& p: progs) {
    for (const Source& s: p.sources) {
      if (!p.prog->addCacheableShaderFromSourceFile(s.stype, s.fname)) {
        qFatal("Failed to compile %s: %s", s.fname.toUtf8().data(), p.prog->log().toUtf8().data());
      }
    }
    if (!p.prog->link()) {
      qFatal("Failed to link the chartpainter program: %s", p.prog->log().toUtf8().data());
    }
  }


  updateCharts();

  // locations
  m_program->bind();
  m_tri_locations.base_color = m_program->uniformLocation("base_color");
  m_tri_locations.m_pvm = m_program->uniformLocation("m_pvm");
  m_tri_locations.depth = m_program->uniformLocation("depth");

  m_lineProg->bind();
  m_line_locations.base_color = m_lineProg->uniformLocation("base_color");
  m_line_locations.m_pvm = m_lineProg->uniformLocation("m_pvm");
  m_line_locations.depth = m_lineProg->uniformLocation("depth");
  m_line_locations.lineWidth = m_lineProg->uniformLocation("lineWidth");
  m_line_locations.screenHeight = m_lineProg->uniformLocation("screenHeight");
  m_line_locations.screenWidth = m_lineProg->uniformLocation("screenWidth");
  m_line_locations.pattern = m_lineProg->uniformLocation("pattern");
  m_line_locations.patlen = m_lineProg->uniformLocation("patlen");
  m_line_locations.factor = m_lineProg->uniformLocation("factor");

  auto gl = QOpenGLContext::currentContext()->functions();
  gl->glEnable(GL_DEPTH_TEST);
  gl->glDisable(GL_STENCIL_TEST);
  gl->glDisable(GL_CULL_FACE);
}


void ChartPainter::paintGL(const Camera* cam) {
  auto gl = QOpenGLContext::currentContext()->extraFunctions();

  m_coordBuffer.bind();
  m_indexBuffer.bind();

  QVector<QMatrix4x4> modelTransforms;
  for (const WGS84Point& q: m_transforms) {
    QPointF p = cam->geoprojection()->fromWGS84(q);
    QMatrix4x4 tr;
    tr.translate(p.x(), p.y());
    modelTransforms.append(tr);
  }

  // draw with non-discarding progs nearest objects (highest priority) first
  m_program->bind();
  m_program->enableAttributeArray(0);
  for (int i = S52::Lookup::PriorityCount - 1; i >= 0; i--) {
    for (int j = 0; j < m_chartData[i].size(); j++) {

      const ChartData& d = m_chartData[i][j];
      if (d.triangleData.isEmpty()) continue;

      const QMatrix4x4 tr = modelTransforms[j];
      m_program->setUniformValue(m_tri_locations.m_pvm,
                                 cam->projection() * cam->view() * tr);
      m_program->setUniformValue(m_tri_locations.depth, GLfloat(- 1 + 0.1 * i + 0.01));

      for (const S57::PaintData& item: d.triangleData) {

        m_program->setUniformValue(m_tri_locations.base_color, item.color);
        m_program->setAttributeBuffer(0, GL_FLOAT, d.vertexOffset + item.vertexOffset, 2, 0);
        GLint first = 0;
        for (const S57::ElementData& e: item.elements) {
          gl->glDrawArrays(e.mode, first, e.elementCount);
          first += e.elementCount;
        }
      }
    }
  }

  // draw with discarding progs in any order
  m_lineProg->bind();
  m_lineProg->enableAttributeArray(0);

  const float ph = cam->heightMM() / 25.4 *
      QGuiApplication::primaryScreen()->physicalDotsPerInchY();
  const float pw = cam->heightMM() * cam->aspect() / 25.4 *
      QGuiApplication::primaryScreen()->physicalDotsPerInchX();

  m_lineProg->setUniformValue(m_line_locations.screenHeight, ph);
  m_lineProg->setUniformValue(m_line_locations.screenWidth, pw);
  m_lineProg->setUniformValue(m_line_locations.patlen, linePatlen);
  m_lineProg->setUniformValue(m_line_locations.factor, linefactor);

  for (int i = 0; i < S52::Lookup::PriorityCount; i++) {
    for (int j = 0; j < m_chartData[i].size(); j++) {

      const ChartData& d = m_chartData[i][j];
      if (d.lineData.isEmpty()) continue;

      const QMatrix4x4 tr = modelTransforms[j];
      m_lineProg->setUniformValue(m_line_locations.m_pvm,
                                  cam->projection() * cam->view() * tr);
      m_lineProg->setUniformValue(m_line_locations.depth, GLfloat(- 1 + 0.1 * i + 0.02));

      for (const S57::PaintData& item: d.lineData) {

        m_lineProg->setUniformValue(m_line_locations.base_color, item.color);
        m_lineProg->setUniformValue(m_line_locations.lineWidth, (GLfloat) item.params.line.lineWidth);
        // m_lineProg->setUniformValue(m_line_locations.lineWidth, (GLfloat) 1.);
        m_lineProg->setUniformValue(m_line_locations.pattern, item.params.line.pattern);

        m_lineProg->setAttributeBuffer(0, GL_FLOAT, d.vertexOffset + item.vertexOffset, 2, 0);

        for (const S57::ElementData& e: item.elements) {
          gl->glDrawElements(GL_LINE_STRIP_ADJACENCY, e.elementCount, GL_UNSIGNED_INT,
                             (const void*) (d.elementOffset + e.elementOffset));
        }
      }
    }
  }


}

void ChartPainter::updateBuffers() {
  QVector<GLfloat> vertices;
  QVector<GLuint> indices;

  // qDebug() << "updateBuffers" << m_manager->charts().size();

  m_transforms.clear();
  for (S57Chart* chart: m_manager->charts()) {
    vertices.append(chart->vertices());
    indices.append(chart->indices());
    m_transforms.append(chart->geoProjection()->reference());
  }

  if (!m_coordBuffer.isCreated()) {
    m_coordBuffer.create();
    m_coordBuffer.setUsagePattern(QOpenGLBuffer::DynamicDraw);
  }
  m_coordBuffer.bind();
  GLsizei dataLen = vertices.size() * sizeof(GLfloat);
  m_coordBuffer.allocate(dataLen);
  m_coordBuffer.write(0, vertices.constData(), dataLen);

  if (!m_indexBuffer.isCreated()) {
    m_indexBuffer.create();
    m_indexBuffer.setUsagePattern(QOpenGLBuffer::DynamicDraw);
  }
  m_indexBuffer.bind();
  GLsizei elemLen = indices.size() * sizeof(GLuint);
  m_indexBuffer.allocate(elemLen);
  m_indexBuffer.write(0, indices.constData(), elemLen);
}


void ChartPainter::updateObjects() {
  uintptr_t elementOffset = 0;
  uintptr_t vertexOffset = 0;

  // qDebug() << "updateObjects" << m_manager->charts().size();
  for (QVector<ChartData>& d: m_chartData) d.clear();

  for (S57Chart* chart: m_manager->charts()) {
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
}
