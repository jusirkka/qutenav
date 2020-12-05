#include "shader.h"
#include "camera.h"
#include <QGuiApplication>
#include <QScreen>
#include <QDebug>
#include "textmanager.h"

GL::Shader::Shader(const QVector<Source>& sources, GLfloat ds)
  : m_depthShift(ds)
{
  m_program = new QOpenGLShaderProgram;

  for (const Source& s: sources) {
    if (!m_program->addCacheableShaderFromSourceFile(s.stype, s.fname)) {
      qFatal("Failed to compile %s: %s", s.fname.toUtf8().data(), m_program->log().toUtf8().data());
    }
  }

  if (!m_program->link()) {
    qFatal("Failed to link the outliner program: %s", m_program->log().toUtf8().data());
  }

  m_program->bind();
  m_depth = m_program->uniformLocation("depth");
}

void GL::Shader::setDepth(int prio) {
  m_program->setUniformValue(m_depth, -1.f + .1f * prio + m_depthShift);
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

void GL::AreaShader::setGlobals(const Camera *cam, const QPointF &tr) {
  m_program->setUniformValue(m_locations.m_pv, cam->projection() * cam->view());
  m_program->setUniformValue(m_locations.tr, tr);
}


GL::AreaShader::AreaShader()
  : Shader({{QOpenGLShader::Vertex, ":/shaders/chartpainter.vert"},
            {QOpenGLShader::Fragment, ":/shaders/chartpainter.frag"}}, .01)
{
  m_locations.base_color = m_program->uniformLocation("base_color");
  m_locations.m_pv = m_program->uniformLocation("m_pv");
  m_locations.tr = m_program->uniformLocation("tr");
}

GL::SolidLineShader* GL::SolidLineShader::instance() {
  static auto shader = new GL::SolidLineShader;
  return shader;
}

void GL::SolidLineShader::setGlobals(const Camera *cam, const QPointF &tr) {
  m_program->setUniformValue(m_locations.m_pv, cam->projection() * cam->view());
  m_program->setUniformValue(m_locations.tr, tr);
  const float s = .5 * cam->heightMM() * m_dots_per_mm_y * cam->projection()(1, 1);
  m_program->setUniformValue(m_locations.windowScale, s);
}


GL::SolidLineShader::SolidLineShader()
  : Shader({{QOpenGLShader::Vertex, ":/shaders/chartpainter-lines.vert"},
            {QOpenGLShader::Geometry, ":/shaders/chartpainter-lines.geom"},
            {QOpenGLShader::Fragment, ":/shaders/chartpainter.frag"}}, .02)
  , m_dots_per_mm_y(QGuiApplication::primaryScreen()->physicalDotsPerInchY() / 25.4)
{
  m_locations.m_pv = m_program->uniformLocation("m_pv");
  m_locations.tr = m_program->uniformLocation("tr");
  m_locations.windowScale = m_program->uniformLocation("windowScale");
  m_locations.lineWidth = m_program->uniformLocation("lineWidth");
  m_locations.base_color = m_program->uniformLocation("base_color");
}


GL::DashedLineShader* GL::DashedLineShader::instance() {
  static auto shader = new GL::DashedLineShader;
  return shader;
}


void GL::DashedLineShader::setGlobals(const Camera *cam, const QPointF &tr) {
  m_program->setUniformValue(m_locations.m_pv, cam->projection() * cam->view());
  m_program->setUniformValue(m_locations.tr, tr);
  const float s = .5 * cam->heightMM() * m_dots_per_mm_y * cam->projection()(1, 1);
  m_program->setUniformValue(m_locations.windowScale, s);
  m_program->setUniformValue(m_locations.patlen, linePatlen);
  m_program->setUniformValue(m_locations.factor, linefactor);
}


GL::DashedLineShader::DashedLineShader()
  : Shader({{QOpenGLShader::Vertex, ":/shaders/chartpainter-lines.vert"},
            {QOpenGLShader::Geometry, ":/shaders/chartpainter-dashed.geom"},
            {QOpenGLShader::Fragment, ":/shaders/chartpainter-dashed.frag"}}, .02)
  , m_dots_per_mm_y(QGuiApplication::primaryScreen()->physicalDotsPerInchY() / 25.4)
{
  m_locations.m_pv = m_program->uniformLocation("m_pv");
  m_locations.tr = m_program->uniformLocation("tr");
  m_locations.windowScale = m_program->uniformLocation("windowScale");
  m_locations.lineWidth = m_program->uniformLocation("lineWidth");
  m_locations.base_color = m_program->uniformLocation("base_color");
  m_locations.pattern = m_program->uniformLocation("pattern");
  m_locations.patlen = m_program->uniformLocation("patlen");
  m_locations.factor = m_program->uniformLocation("factor");
}


GL::TextShader* GL::TextShader::instance() {
  static auto shader = new GL::TextShader;
  return shader;
}

void GL::TextShader::setGlobals(const Camera *cam, const QPointF &tr) {
  m_program->setUniformValue(m_locations.m_pv, cam->projection() * cam->view());
  m_program->setUniformValue(m_locations.tr, tr);
  const float s = .5 * cam->heightMM() * m_dots_per_mm_y * cam->projection()(1, 1);
  m_program->setUniformValue(m_locations.windowScale, s);
  m_program->setUniformValue(m_locations.atlas, 0);
  m_program->setUniformValue(m_locations.w_atlas, TextManager::instance()->atlasWidth());
  m_program->setUniformValue(m_locations.h_atlas, TextManager::instance()->atlasHeight());
}


GL::TextShader::TextShader()
  : Shader({{QOpenGLShader::Vertex, ":/shaders/chartpainter-text.vert"},
            {QOpenGLShader::Fragment, ":/shaders/chartpainter-text.frag"}}, .04)
  , m_dots_per_mm_y(QGuiApplication::primaryScreen()->physicalDotsPerInchY() / 25.4)
{
  m_locations.m_pv = m_program->uniformLocation("m_pv");
  m_locations.tr = m_program->uniformLocation("tr");
  m_locations.w_atlas = m_program->uniformLocation("w_atlas");
  m_locations.h_atlas = m_program->uniformLocation("h_atlas");
  m_locations.windowScale = m_program->uniformLocation("windowScale");
  m_locations.textScale = m_program->uniformLocation("textScale");
  m_locations.pivot = m_program->uniformLocation("pivot");
  m_locations.pivotShift = m_program->uniformLocation("pivotShift");
  m_locations.atlas = m_program->uniformLocation("atlas");
  m_locations.base_color = m_program->uniformLocation("base_color");
}

void GL::TextShader::initializePaint() {
  m_program->bind();
  m_program->enableAttributeArray(0);
  m_program->enableAttributeArray(1);
}


GL::RasterSymbolShader* GL::RasterSymbolShader::instance() {
  static auto shader = new GL::RasterSymbolShader;
  return shader;
}

void GL::RasterSymbolShader::setGlobals(const Camera *cam, const QPointF &tr) {
  m_program->setUniformValue(m_locations.m_pv, cam->projection() * cam->view());
  m_program->setUniformValue(m_locations.tr, tr);
  const float s = .5 * cam->heightMM() * m_dots_per_mm_y * cam->projection()(1, 1);
  m_program->setUniformValue(m_locations.windowScale, s);
  m_program->setUniformValue(m_locations.atlas, 0);

  const int texOffset = 2 * sizeof(GLfloat);
  const int stride = 4 * sizeof(GLfloat);
  m_program->setAttributeBuffer(0, GL_FLOAT, 0, 2, stride);
  m_program->setAttributeBuffer(1, GL_FLOAT, texOffset, 2, stride);

}


GL::RasterSymbolShader::RasterSymbolShader()
  : Shader({{QOpenGLShader::Vertex, ":/shaders/chartpainter-rastersymbol.vert"},
            {QOpenGLShader::Fragment, ":/shaders/chartpainter-rastersymbol.frag"}}, .03)
  , m_dots_per_mm_y(QGuiApplication::primaryScreen()->physicalDotsPerInchY() / 25.4)
{
  m_locations.m_pv = m_program->uniformLocation("m_pv");
  m_locations.tr = m_program->uniformLocation("tr");
  m_locations.windowScale = m_program->uniformLocation("windowScale");
  m_locations.pivot = m_program->uniformLocation("pivot");
  m_locations.pivotShift = m_program->uniformLocation("pivotShift");
  m_locations.atlas = m_program->uniformLocation("atlas");
}

void GL::RasterSymbolShader::initializePaint() {
  m_program->bind();
  m_program->enableAttributeArray(0);
  m_program->enableAttributeArray(1);
}
