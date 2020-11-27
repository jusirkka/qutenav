#include "shader.h"
#include "camera.h"
#include <QGuiApplication>
#include <QScreen>


GL::Shader::Shader(const QVector<Source>& sources) {
  m_program = new QOpenGLShaderProgram;

  for (const Source& s: sources) {
    if (!m_program->addCacheableShaderFromSourceFile(s.stype, s.fname)) {
      qFatal("Failed to compile %s: %s", s.fname.toUtf8().data(), m_program->log().toUtf8().data());
    }
  }

  if (!m_program->link()) {
    qFatal("Failed to link the outliner program: %s", m_program->log().toUtf8().data());
  }
}

GL::Shader::~Shader() {
  delete m_program;
}


void GL::Shader::initializePaint() {
  m_program->bind();
  m_program->enableAttributeArray(0);
}

GL::AreaShader* GL::AreaShader::instance() {
  static auto shader = new GL::AreaShader;
  return shader;
}

void GL::AreaShader::setTransform(const QMatrix4x4& pvm) {
  m_program->setUniformValue(m_locations.m_pvm, pvm);
}

void GL::AreaShader::setDepth(GLfloat depth) {
  m_program->setUniformValue(m_locations.depth, depth);
}


GL::AreaShader::AreaShader()
  : Shader({{QOpenGLShader::Vertex, ":/shaders/chartpainter.vert"},
            {QOpenGLShader::Fragment, ":/shaders/chartpainter.frag"}})
{

  m_program->bind();

  m_locations.base_color = m_program->uniformLocation("base_color");
  m_locations.m_pvm = m_program->uniformLocation("m_pvm");
  m_locations.depth = m_program->uniformLocation("depth");

}



GL::SolidLineShader* GL::SolidLineShader::instance() {
  static auto shader = new GL::SolidLineShader;
  return shader;
}

void GL::SolidLineShader::setTransform(const QMatrix4x4& pvm) {
  m_program->setUniformValue(m_locations.m_pvm, pvm);
}

void GL::SolidLineShader::setDepth(GLfloat depth) {
  m_program->setUniformValue(m_locations.depth, depth);
}

void GL::SolidLineShader::setScreen(const Camera* cam) {

  const float hw = cam->heightMM() * cam->aspect() * m_dots_per_mm_x * .5;
  m_program->setUniformValue(m_locations.screenXMax, hw);

  const float hh = cam->heightMM() * m_dots_per_mm_y * .5;
  m_program->setUniformValue(m_locations.screenYMax, hh);
}

GL::SolidLineShader::SolidLineShader()
  : Shader({{QOpenGLShader::Vertex, ":/shaders/chartpainter.vert"},
            {QOpenGLShader::Geometry, ":/shaders/chartpainter-lines.geom"},
            {QOpenGLShader::Fragment, ":/shaders/chartpainter.frag"}})
  , m_dots_per_mm_x(QGuiApplication::primaryScreen()->physicalDotsPerInchX() / 25.4)
  , m_dots_per_mm_y(QGuiApplication::primaryScreen()->physicalDotsPerInchY() / 25.4)
{
  m_program->bind();

  m_locations.base_color = m_program->uniformLocation("base_color");
  m_locations.m_pvm = m_program->uniformLocation("m_pvm");
  m_locations.depth = m_program->uniformLocation("depth");
  m_locations.lineWidth = m_program->uniformLocation("lineWidth");
  m_locations.screenXMax = m_program->uniformLocation("screenXMax");
  m_locations.screenYMax = m_program->uniformLocation("screenYMax");
}


GL::DashedLineShader* GL::DashedLineShader::instance() {
  static auto shader = new GL::DashedLineShader;
  return shader;
}

void GL::DashedLineShader::setTransform(const QMatrix4x4& pvm) {
  m_program->setUniformValue(m_locations.m_pvm, pvm);
}

void GL::DashedLineShader::setDepth(GLfloat depth) {
  m_program->setUniformValue(m_locations.depth, depth);
}

void GL::DashedLineShader::setScreen(const Camera* cam) {

  const float hw = cam->heightMM() * cam->aspect() * m_dots_per_mm_x * .5;
  m_program->setUniformValue(m_locations.screenXMax, hw);

  const float hh = cam->heightMM() * m_dots_per_mm_y * .5;
  m_program->setUniformValue(m_locations.screenYMax, hh);

  m_program->setUniformValue(m_locations.patlen, linePatlen);
  m_program->setUniformValue(m_locations.factor, linefactor);
}

GL::DashedLineShader::DashedLineShader()
  : Shader({{QOpenGLShader::Vertex, ":/shaders/chartpainter.vert"},
            {QOpenGLShader::Geometry, ":/shaders/chartpainter-dashed.geom"},
            {QOpenGLShader::Fragment, ":/shaders/chartpainter-dashed.frag"}})
  , m_dots_per_mm_x(QGuiApplication::primaryScreen()->physicalDotsPerInchX() / 25.4)
  , m_dots_per_mm_y(QGuiApplication::primaryScreen()->physicalDotsPerInchY() / 25.4)
{
  m_program->bind();

  m_locations.base_color = m_program->uniformLocation("base_color");
  m_locations.m_pvm = m_program->uniformLocation("m_pvm");
  m_locations.depth = m_program->uniformLocation("depth");
  m_locations.lineWidth = m_program->uniformLocation("lineWidth");
  m_locations.screenXMax = m_program->uniformLocation("screenXMax");
  m_locations.screenYMax = m_program->uniformLocation("screenYMax");
  m_locations.pattern = m_program->uniformLocation("pattern");
  m_locations.patlen = m_program->uniformLocation("patlen");
  m_locations.factor = m_program->uniformLocation("factor");
}

