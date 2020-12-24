#include "linecalculator.h"
#include <QOpenGLExtraFunctions>
#include <QOpenGLDebugLogger>

GL::LineCalculator* GL::LineCalculator::instance() {
  static LineCalculator* lc = new LineCalculator();
  return lc;
}

GL::LineCalculator::LineCalculator()
  : m_program(new QOpenGLShaderProgram())
  , m_logger(new QOpenGLDebugLogger())
  , m_outBuffer(QOpenGLBuffer::VertexBuffer)
{

  if (!m_logger->initialize()) {
    qWarning() << "OpenGL logging not available";
  }
  m_logger->disableMessages(QOpenGLDebugMessage::APISource,
                            QOpenGLDebugMessage::PerformanceType);


  if (!m_program->addCacheableShaderFromSourceFile(QOpenGLShader::Compute, ":/shaders/linecalculator.comp")) {
    qFatal("Failed to compile compute shader: %s", m_program->log().toUtf8().data());
  }

  if (!m_program->link()) {
    qFatal("Failed to link the linecalculator program: %s", m_program->log().toUtf8().data());
  }

  // locations
  m_program->bind();
  m_locations.period = m_program->uniformLocation("period");
  m_locations.viewArea = m_program->uniformLocation("viewArea");
  m_locations.numOutIndices = m_program->uniformLocation("numOutIndices");
  m_locations.indexOffset = m_program->uniformLocation("indexOffset");
  m_locations.vertexOffset = m_program->uniformLocation("vertexOffset");

  m_outBuffer.create();
  m_outBuffer.bind();
  m_outBuffer.setUsagePattern(QOpenGLBuffer::DynamicDraw);
}

GL::LineCalculator::~LineCalculator() {
  delete m_program;
  delete m_logger;
}

void GL::LineCalculator::calculate(VertexVector& transforms,
                                   GLfloat period,
                                   const QRectF& va,
                                   BufferData& vertices,
                                   BufferData& indices) {
  if (period < 1.e-10) {
    qWarning() << "GL::LineCalculator: Period is too small" << period;
    return;
  }

  m_program->bind();

  m_program->setUniformValue(m_locations.period, period);
  const QVector4D vav(va.left(), va.top(), va.left() + va.width(), va.top() + va.height());
  m_program->setUniformValue(m_locations.viewArea, vav);

  const int numOutIndices = indices.count - 1;
  m_program->setUniformValue(m_locations.numOutIndices, numOutIndices);
  m_program->setUniformValue(m_locations.indexOffset, indices.offset);
  m_program->setUniformValue(m_locations.vertexOffset, vertices.offset);

  // initialize outbuffer
  m_outBuffer.bind();
  // num, ca, sa, pad, first * 2, dir * 2
  int dataLen = numOutIndices * sizeof(GLfloat) * 8;
  if (m_outBuffer.size() < dataLen) {
    m_outBuffer.allocate(dataLen);
  }

  auto f = QOpenGLContext::currentContext()->extraFunctions();

  // bind buffers
  f->glBindBufferBase(GL_SHADER_STORAGE_BUFFER,
                      compVertexBufferInBinding,
                      vertices.buffer.bufferId());
  f->glBindBufferBase(GL_SHADER_STORAGE_BUFFER,
                      compIndexBufferInBinding,
                      indices.buffer.bufferId());
  f->glBindBufferBase(GL_SHADER_STORAGE_BUFFER,
                      compVertexBufferOutBinding,
                      m_outBuffer.bufferId());

  f->glDispatchCompute((numOutIndices + 63) / 64, 1, 1);

  for (const QOpenGLDebugMessage& message: m_logger->loggedMessages()) {
    qDebug() << message;
  }

  m_outBuffer.bind();
  auto src = reinterpret_cast<const Transform*>(m_outBuffer.mapRange(0, numOutIndices * sizeof(Transform), QOpenGLBuffer::RangeRead));

  if (src == nullptr) {
    qWarning() << "GL::LineCalculator: No output";
    return;
  }

  createTransforms(transforms, src, numOutIndices);
  m_outBuffer.unmap();
}

void GL::LineCalculator::createTransforms(VertexVector &tr, const Transform *source, int numIndices) const {
  for (int i = 0; i < numIndices; i++) {
    const Transform& src = source[i];
    for (int n = 0; n < src.num; n++) {
      const glm::vec2 v = src.first + (1.f * n) * src.dir;
      tr << v.x << v.y << src.ca << src.sa;
    }
  }
}



