#include "shader.h"
#include "camera.h"
#include <QDebug>
#include "textmanager.h"
#include "platform.h"
#include <QOpenGLExtraFunctions>

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
    qFatal("Failed to link the program: %s", m_program->log().toUtf8().data());
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

void GL::AreaShader::setGlobals(const Camera *cam, const QMatrix4x4 &mt) {
  m_program->setUniformValue(m_locations.m_p, cam->projection());
  m_program->setUniformValue(m_locations.m_model, mt);
}


GL::AreaShader::AreaShader()
  : Shader({{QOpenGLShader::Vertex, ":/shaders/chartpainter.vert"},
            {QOpenGLShader::Fragment, ":/shaders/chartpainter.frag"}}, .01)
{
  m_locations.base_color = m_program->uniformLocation("base_color");
  m_locations.m_p = m_program->uniformLocation("m_p");
  m_locations.m_model = m_program->uniformLocation("m_model");
}

GL::SolidLineShader* GL::SolidLineShader::instance() {
  static auto shader = new GL::SolidLineShader;
  return shader;
}

void GL::SolidLineShader::setGlobals(const Camera *cam, const QMatrix4x4 &mt) {
  m_program->setUniformValue(m_locations.m_p, cam->projection());
  m_program->setUniformValue(m_locations.m_model, mt);
  const float s = .5 * cam->heightMM() * dots_per_mm_y * cam->projection()(1, 1);
  m_program->setUniformValue(m_locations.windowScale, s);
}


GL::SolidLineShader::SolidLineShader()
  : Shader({{QOpenGLShader::Vertex, ":/shaders/chartpainter-lines.vert"},
            {QOpenGLShader::Geometry, ":/shaders/chartpainter-lines.geom"},
            {QOpenGLShader::Fragment, ":/shaders/chartpainter.frag"}}, .02)
{
  m_locations.m_p = m_program->uniformLocation("m_p");
  m_locations.m_model = m_program->uniformLocation("m_model");
  m_locations.windowScale = m_program->uniformLocation("windowScale");
  m_locations.lineWidth = m_program->uniformLocation("lineWidth");
  m_locations.base_color = m_program->uniformLocation("base_color");
}


GL::DashedLineShader* GL::DashedLineShader::instance() {
  static auto shader = new GL::DashedLineShader;
  return shader;
}


void GL::DashedLineShader::setGlobals(const Camera *cam, const QMatrix4x4 &mt) {
  m_program->setUniformValue(m_locations.m_p, cam->projection());
  m_program->setUniformValue(m_locations.m_model, mt);
  const float s = .5 * cam->heightMM() * dots_per_mm_y * cam->projection()(1, 1);
  m_program->setUniformValue(m_locations.windowScale, s);
  m_program->setUniformValue(m_locations.patlen, linePatlen);
  m_program->setUniformValue(m_locations.factor, linefactor);
}


GL::DashedLineShader::DashedLineShader()
  : Shader({{QOpenGLShader::Vertex, ":/shaders/chartpainter-lines.vert"},
            {QOpenGLShader::Geometry, ":/shaders/chartpainter-dashed.geom"},
            {QOpenGLShader::Fragment, ":/shaders/chartpainter-dashed.frag"}}, .02)
{
  m_locations.m_p = m_program->uniformLocation("m_p");
  m_locations.m_model = m_program->uniformLocation("m_model");
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

void GL::TextShader::setGlobals(const Camera *cam, const QMatrix4x4 &mt) {
  m_program->setUniformValue(m_locations.m_p, cam->projection());
  m_program->setUniformValue(m_locations.m_model, mt);
  const float s = .5 * cam->heightMM() * dots_per_mm_y * cam->projection()(1, 1);
  m_program->setUniformValue(m_locations.windowScale, s);
  m_program->setUniformValue(m_locations.w_atlas, TextManager::instance()->atlasWidth());
  m_program->setUniformValue(m_locations.h_atlas, TextManager::instance()->atlasHeight());
}


GL::TextShader::TextShader()
  : Shader({{QOpenGLShader::Vertex, ":/shaders/chartpainter-text.vert"},
            {QOpenGLShader::Fragment, ":/shaders/chartpainter-text.frag"}}, .04)
{
  m_locations.m_p = m_program->uniformLocation("m_p");
  m_locations.m_model = m_program->uniformLocation("m_model");
  m_locations.w_atlas = m_program->uniformLocation("w_atlas");
  m_locations.h_atlas = m_program->uniformLocation("h_atlas");
  m_locations.windowScale = m_program->uniformLocation("windowScale");
  m_locations.textScale = m_program->uniformLocation("textScale");
  m_locations.pivot = m_program->uniformLocation("pivot");
  m_locations.offset = m_program->uniformLocation("offset");
  m_locations.base_color = m_program->uniformLocation("base_color");
}

void GL::TextShader::initializePaint() {
  m_program->bind();
  m_program->enableAttributeArray(0);
  m_program->enableAttributeArray(1);
  auto f = QOpenGLContext::currentContext()->extraFunctions();
  f->glVertexAttribDivisor(1, 0);
}


GL::RasterSymbolShader* GL::RasterSymbolShader::instance() {
  static auto shader = new GL::RasterSymbolShader;
  return shader;
}

void GL::RasterSymbolShader::setGlobals(const Camera *cam, const QMatrix4x4 &mt) {
  m_program->setUniformValue(m_locations.m_p, cam->projection());
  m_program->setUniformValue(m_locations.m_model, mt);
  const float s = .5 * cam->heightMM() * dots_per_mm_y * cam->projection()(1, 1);
  m_program->setUniformValue(m_locations.windowScale, s);

  const int texOffset = 2 * sizeof(GLfloat);
  const int stride = 4 * sizeof(GLfloat);
  m_program->setAttributeBuffer(0, GL_FLOAT, 0, 2, stride);
  m_program->setAttributeBuffer(1, GL_FLOAT, texOffset, 2, stride);
}


GL::RasterSymbolShader::RasterSymbolShader()
  : Shader({{QOpenGLShader::Vertex, ":/shaders/chartpainter-rastersymbol.vert"},
            {QOpenGLShader::Fragment, ":/shaders/chartpainter-texture.frag"}}, .03)
{
  m_locations.m_p = m_program->uniformLocation("m_p");
  m_locations.m_model = m_program->uniformLocation("m_model");
  m_locations.windowScale = m_program->uniformLocation("windowScale");
  m_locations.offset = m_program->uniformLocation("offset");
}

void GL::RasterSymbolShader::initializePaint() {
  m_program->bind();
  m_program->enableAttributeArray(0);
  m_program->enableAttributeArray(1);
  m_program->enableAttributeArray(2);
  auto f = QOpenGLContext::currentContext()->extraFunctions();
  f->glVertexAttribDivisor(1, 0);
  f->glVertexAttribDivisor(2, 1);
}

GL::VectorSymbolShader* GL::VectorSymbolShader::instance() {
  static auto shader = new GL::VectorSymbolShader;
  return shader;
}

void GL::VectorSymbolShader::setGlobals(const Camera *cam, const QMatrix4x4 &mt) {
  m_program->setUniformValue(m_locations.m_p, cam->projection());
  m_program->setUniformValue(m_locations.m_model, mt);

  // Vector symbol data specified in .01 mm units
  const float s = 100 * cam->heightMM() * cam->projection()(1, 1);
  m_program->setUniformValue(m_locations.windowScale, s);

  m_program->setAttributeBuffer(0, GL_FLOAT, 0, 2, 0);
}


GL::VectorSymbolShader::VectorSymbolShader()
  : Shader({{QOpenGLShader::Vertex, ":/shaders/chartpainter-vectorsymbol.vert"},
            {QOpenGLShader::Fragment, ":/shaders/chartpainter.frag"}}, .03)
{
  m_locations.m_p = m_program->uniformLocation("m_p");
  m_locations.m_model = m_program->uniformLocation("m_model");
  m_locations.base_color = m_program->uniformLocation("base_color");
  m_locations.windowScale = m_program->uniformLocation("windowScale");
}

void GL::VectorSymbolShader::initializePaint() {
  m_program->bind();
  m_program->enableAttributeArray(0);
  m_program->enableAttributeArray(1);
  auto f = QOpenGLContext::currentContext()->extraFunctions();
  f->glVertexAttribDivisor(1, 1);
}


GL::TextureShader* GL::TextureShader::instance() {
  static auto shader = new GL::TextureShader;
  return shader;
}

void GL::TextureShader::setGlobals(const Camera */*cam*/, const QMatrix4x4 &/*mt*/) {
  // noop
}

void GL::TextureShader::setUniforms(const Camera* cam,
                                    const WGS84Point& ref,
                                    const QRectF& va) {

  m_program->setUniformValue(m_locations.m_pv, cam->projection() * cam->view());

  const QRectF vp0 = cam->boundingBox().translated(- cam->geoprojection()->fromWGS84(ref));

  const qreal lx = .5 * vp0.width();
  const qreal ly = .5 * vp0.height();

  const QRectF vp = va.intersected(vp0);
  const qreal dx0 = vp0.left() - vp.left();
  const qreal dx1 = vp0.right() - vp.right();
  const qreal dx = dx0 == 0. ? dx1 : dx0;

  const qreal dy0 = vp0.top() - vp.top();
  const qreal dy1 = vp0.bottom() - vp.bottom();
  const qreal dy = dy0 == 0. ? dy1 : dy0;

  m_program->setUniformValue(m_locations.scale_vertex, QPointF(lx, ly));
  m_program->setUniformValue(m_locations.tr_vertex, QPointF(-dx, -dy));

  if (vp != vp0) {
    qWarning() << "bad rectangles" << vp << vp0;
  }

  const QPointF p1 = vp0.topLeft();
  const QPointF q1(p1.x() / va.width(), p1.y() / va.height());

  const QPointF p2 = vp0.bottomRight();
  const QPointF q2(p2.x() / va.width(), p2.y() / va.height());

  const QPointF tr = q1 + QPointF(.5, .5);
  const QPointF sc = (q2 - q1);

  const qreal s = qMax(0., qMin(1. - sc.x(), tr.x()));
  const qreal t = qMax(0., qMin(1. - sc.y(), tr.y()));

  m_program->setUniformValue(m_locations.tr_tex, QPointF(s, t));
  m_program->setUniformValue(m_locations.scale_tex, sc);

  const int texOffset = 2 * sizeof(GLfloat);
  const int stride = 4 * sizeof(GLfloat);
  m_program->setAttributeBuffer(0, GL_FLOAT, 0, 2, stride);
  m_program->setAttributeBuffer(1, GL_FLOAT, texOffset, 2, stride);
}

GL::TextureShader::TextureShader()
  : Shader({{QOpenGLShader::Vertex, ":/shaders/chartpainter-texture.vert"},
            {QOpenGLShader::Fragment, ":/shaders/chartpainter-texture.frag"}}, .0)
{
  m_locations.m_pv = m_program->uniformLocation("m_pv");
  m_locations.scale_tex = m_program->uniformLocation("scale_tex");
  m_locations.tr_tex = m_program->uniformLocation("tr_tex");
  m_locations.scale_vertex = m_program->uniformLocation("scale_vertex");
  m_locations.tr_vertex = m_program->uniformLocation("tr_vertex");
}

void GL::TextureShader::initializePaint() {
  m_program->bind();
  m_program->enableAttributeArray(0);
  m_program->enableAttributeArray(1);
  auto f = QOpenGLContext::currentContext()->extraFunctions();
  f->glVertexAttribDivisor(1, 0);
}
