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
  , m_coordBuffer(QOpenGLBuffer::VertexBuffer)
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

    m_coordBuffer.create();
    m_coordBuffer.setUsagePattern(QOpenGLBuffer::DynamicDraw);

    // locations
    m_program->bind();
    m_locations.base_color = m_program->uniformLocation("base_color");
    m_locations.m_pv = m_program->uniformLocation("m_pv");
    m_locations.center = m_program->uniformLocation("center");
    m_locations.angle = m_program->uniformLocation("angle");
    m_locations.vertexOffset = m_program->uniformLocation("vertexOffset");

  }

  updateBuffers();
}

void Outliner::updateCharts(const Camera* /*cam*/, const QRectF& /*viewArea*/) {
  // noop
}

void Outliner::paintGL(const Camera* cam) {
  auto f = QOpenGLContext::currentContext()->extraFunctions();

  f->glEnable(GL_DEPTH_TEST);
  f->glDisable(GL_BLEND);
  f->glDisable(GL_STENCIL_TEST);
  f->glEnable(GL_CULL_FACE);
  f->glFrontFace(GL_CCW);
  f->glCullFace(GL_BACK);

  m_program->bind();

  // data buffers
  f->glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_coordBuffer.bufferId());

  // uniforms
  m_program->setUniformValue(m_locations.m_pv, cam->projection() * cam->view());
  GLfloat angle = qMax(1.e-5, GLfloat(cam->scale()) / cam->maxScale() * 0.005);
  m_program->setUniformValue(m_locations.angle, angle);

  for (const Rectangle& outline: m_outlines) {
    m_program->setUniformValue(m_locations.base_color, outline.color);
    m_program->setUniformValue(m_locations.center, outline.center);
    m_program->setUniformValue(m_locations.vertexOffset, outline.offset);
    f->glDrawArrays(GL_TRIANGLE_STRIP, 0, 2 * outline.outline.size());
  }
}


void Outliner::updateBuffers() {
  m_coordBuffer.bind();

  m_outlines.clear();

  const int cnt = m_manager->outlines().size() / 8;
  DataVector outlines;
  for (int i = 0; i < cnt; ++i) {
    m_outlines.append(Rectangle(m_manager->outlines(), i, outlines.size()));
    outlines.append(m_outlines.last().outline);
  }

  GLsizei dataLen = outlines.size() * sizeof(glm::vec4);
  m_coordBuffer.allocate(dataLen);
  m_coordBuffer.write(0, outlines.constData(), dataLen);

  qDebug() << "number of rectangles =" << cnt;
}

Outliner::Rectangle::Rectangle(const GL::VertexVector &d, int rectIndex, size_t cnt)
  : color("#ff0000")
  , offset(cnt)
{
  const float eps = 1.e-6;

  int first = rectIndex * 8;
  glm::vec4 sum(0.);
  for (int i = 0; i < 4; i++) {
    const int i1 = (i + 1) % 4;
    const float lng1 = d[first + 2 * i] * M_PI / 180;
    const float lat1 = d[first + 2 * i + 1] * M_PI / 180;
    const float lng2 = d[first + 2 * i1] * M_PI / 180;
    const float lat2 = d[first + 2 * i1 + 1] * M_PI / 180;
    if (std::abs(lng2 - lng1) < eps) {
      Q_ASSERT(std::abs(lat2 - lat1) > eps);
      const int n = qMin(100, qMax(1, static_cast<int>(std::abs(lat2 - lat1) * 90 / M_PI)));
      for (int j = 0; j < n; j++) {
        const float lat = lat1 + j * (lat2 - lat1) / n;
        const glm::vec4 p(1.01 * cos(lng1) * cos(lat),
                          1.01 * sin(lng1) * cos(lat),
                          1.01 * sin(lat),
                          1.);
        outline.append(p);
        if (j == 0) sum += p;
      }
    } else {
      Q_ASSERT(std::abs(lng2 - lng1) > eps);
      const int n = qMin(100, qMax(1, static_cast<int>(std::abs(lng2 - lng1) * 90 / M_PI)));
      for (int j = 0; j < n; j++) {
        const float lng = lng1 + j * (lng2 - lng1) / n;
        const glm::vec4 p(1.01 * cos(lng) * cos(lat1),
                          1.01 * sin(lng) * cos(lat1),
                          1.01 * sin(lat1),
                          1.);
        outline.append(p);
        if (j == 0) sum += p;
      }
    }
  }
  // close the loop
  outline.append(outline.first());

  center = QVector3D(sum.x, sum.y, sum.z).normalized();
}

