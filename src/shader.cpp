/* -*- coding: utf-8-unix -*-
 *
 * File: src/shader.cpp
 *
 * Copyright (C) 2021 Jukka Sirkka
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "shader.h"
#include "camera.h"
#include <QDebug>
#include "textmanager.h"
#include "platform.h"
#include <QOpenGLExtraFunctions>
#include <QFile>
#include "settings.h"

GL::Shader::Shader(const QVector<Source>& sources, int ds)
  : m_depthShift(ds)
{
  m_program = new QOpenGLShaderProgram;

  for (const Source& s: sources) {
    if (!m_program->addShaderFromSourceFile(s.stype, s.fname)) {
      qFatal("Failed to compile %s: %s", s.fname.toUtf8().data(), m_program->log().toUtf8().data());
    }
  }

  if (!m_program->link()) {
    qFatal("Failed to link the program: %s", m_program->log().toUtf8().data());
  }

  m_program->bind();
  m_depth = m_program->uniformLocation("depth");
}

void GL::Shader::setDepth(int chartPrio, int prio) {
  // chartPrio: 0-99
  // prio: 0-9
  // depthShift: 0-9
  m_program->setUniformValue(m_depth, -1.f +
                             .01f * chartPrio +
                             .001f * prio +
                             .0001f * m_depthShift);
}

GL::Shader::~Shader() {
  delete m_program;
}


void GL::Shader::initializePaint() {
  auto f = QOpenGLContext::currentContext()->extraFunctions();
  GLint pid;
  f->glGetIntegerv(GL_CURRENT_PROGRAM, &pid);
  if (pid == static_cast<GLint>(m_program->programId())) return; // already bound
  m_program->bind();
  m_program->enableAttributeArray(0);
  f->glVertexAttribDivisor(0, 0);
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
  : Shader({{QOpenGLShader::Vertex, ":chartpainter.vert"},
            {QOpenGLShader::Fragment, ":chartpainter.frag"}}, 1)
{
  m_locations.base_color = m_program->uniformLocation("base_color");
  m_locations.m_p = m_program->uniformLocation("m_p");
  m_locations.m_model = m_program->uniformLocation("m_model");
}

GL::LineElemShader* GL::LineElemShader::instance() {
  static auto shader = new GL::LineElemShader;
  return shader;
}


void GL::LineElemShader::setGlobals(const Camera *cam, const QMatrix4x4 &mt) {
  m_program->setUniformValue(m_locations.m_p, cam->projection());
  m_program->setUniformValue(m_locations.m_model, mt);
  const float s = .5 * cam->heightMM() * cam->projection()(1, 1);
  m_program->setUniformValue(m_locations.windowScale, s);
}


GL::LineElemShader::LineElemShader()
  : Shader({{QOpenGLShader::Vertex, ":chartpainter-lineelems.vert"},
            {QOpenGLShader::Fragment, ":chartpainter-lines.frag"}}, 2)
{
  m_locations.m_p = m_program->uniformLocation("m_p");
  m_locations.m_model = m_program->uniformLocation("m_model");
  m_locations.windowScale = m_program->uniformLocation("windowScale");
  m_locations.lineWidth = m_program->uniformLocation("lineWidth");
  m_locations.base_color = m_program->uniformLocation("base_color");
  m_locations.pattern = m_program->uniformLocation("pattern");
  m_locations.vertexOffset = m_program->uniformLocation("vertexOffset");
  m_locations.indexOffset = m_program->uniformLocation("indexOffset");
}

GL::LineArrayShader* GL::LineArrayShader::instance() {
  static auto shader = new GL::LineArrayShader;
  return shader;
}


void GL::LineArrayShader::setGlobals(const Camera *cam, const QMatrix4x4 &mt) {
  m_program->setUniformValue(m_locations.m_p, cam->projection());
  m_program->setUniformValue(m_locations.m_model, mt);
  const float s = .5 * cam->heightMM() * cam->projection()(1, 1);
  m_program->setUniformValue(m_locations.windowScale, s);
}


GL::LineArrayShader::LineArrayShader()
  : Shader({{QOpenGLShader::Vertex, ":chartpainter-linearrays.vert"},
            {QOpenGLShader::Fragment, ":chartpainter-lines.frag"}}, 2)
{
  m_locations.m_p = m_program->uniformLocation("m_p");
  m_locations.m_model = m_program->uniformLocation("m_model");
  m_locations.windowScale = m_program->uniformLocation("windowScale");
  m_locations.lineWidth = m_program->uniformLocation("lineWidth");
  m_locations.base_color = m_program->uniformLocation("base_color");
  m_locations.pattern = m_program->uniformLocation("pattern");
  m_locations.vertexOffset = m_program->uniformLocation("vertexOffset");
}

GL::SegmentArrayShader* GL::SegmentArrayShader::instance() {
  static auto shader = new GL::SegmentArrayShader;
  return shader;
}


void GL::SegmentArrayShader::setGlobals(const Camera *cam, const QMatrix4x4 &mt) {
  m_program->setUniformValue(m_locations.m_p, cam->projection());
  m_program->setUniformValue(m_locations.m_model, mt);
  const float s = .5 * cam->heightMM() * cam->projection()(1, 1);
  m_program->setUniformValue(m_locations.windowScale, s);
}


GL::SegmentArrayShader::SegmentArrayShader()
  : Shader({{QOpenGLShader::Vertex, ":chartpainter-segmentarrays.vert"},
            {QOpenGLShader::Fragment, ":chartpainter-lines.frag"}}, 2)
{
  m_locations.m_p = m_program->uniformLocation("m_p");
  m_locations.m_model = m_program->uniformLocation("m_model");
  m_locations.windowScale = m_program->uniformLocation("windowScale");
  m_locations.lineWidth = m_program->uniformLocation("lineWidth");
  m_locations.base_color = m_program->uniformLocation("base_color");
  m_locations.pattern = m_program->uniformLocation("pattern");
  m_locations.vertexOffset = m_program->uniformLocation("vertexOffset");
}

GL::TextShader* GL::TextShader::instance() {
  static auto shader = new GL::TextShader;
  return shader;
}

void GL::TextShader::setGlobals(const Camera *cam, const QMatrix4x4 &mt) {
  m_program->setUniformValue(m_locations.m_p, cam->projection());
  m_program->setUniformValue(m_locations.m_model, mt);
  const float s = .5 * cam->heightMM() * dots_per_mm_y() * cam->projection()(1, 1);
  m_program->setUniformValue(m_locations.windowScale, s);
  m_program->setUniformValue(m_locations.w_atlas, TextManager::instance()->atlasWidth());
  m_program->setUniformValue(m_locations.h_atlas, TextManager::instance()->atlasHeight());
}


GL::TextShader::TextShader()
  : Shader({{QOpenGLShader::Vertex, ":chartpainter-text.vert"},
            {QOpenGLShader::Fragment, ":chartpainter-text.frag"}}, 5)
{
  m_locations.m_p = m_program->uniformLocation("m_p");
  m_locations.m_model = m_program->uniformLocation("m_model");
  m_locations.w_atlas = m_program->uniformLocation("w_atlas");
  m_locations.h_atlas = m_program->uniformLocation("h_atlas");
  m_locations.windowScale = m_program->uniformLocation("windowScale");
  m_locations.base_color = m_program->uniformLocation("base_color");
}

void GL::TextShader::initializePaint() {
  auto f = QOpenGLContext::currentContext()->extraFunctions();
  GLint pid;
  f->glGetIntegerv(GL_CURRENT_PROGRAM, &pid);
  if (pid == static_cast<GLint>(m_program->programId())) return; // already bound
  m_program->bind();
  m_program->enableAttributeArray(0);
  m_program->enableAttributeArray(1);
  m_program->enableAttributeArray(2);
  f->glVertexAttribDivisor(0, 1);
  f->glVertexAttribDivisor(1, 1);
  f->glVertexAttribDivisor(2, 1);
}


GL::RasterSymbolShader* GL::RasterSymbolShader::instance() {
  static auto shader = new GL::RasterSymbolShader;
  return shader;
}

void GL::RasterSymbolShader::setGlobals(const Camera *cam, const QMatrix4x4 &mt) {
  m_program->setUniformValue(m_locations.m_p, cam->projection());
  m_program->setUniformValue(m_locations.m_model, mt);

  const float ds = Settings::instance()->displayRasterSymbolScaling();
  const float s = .5 * cam->heightMM() * cam->projection()(1, 1) * ds;
  m_program->setUniformValue(m_locations.windowScale, s);

  const int texOffset = 2 * sizeof(GLfloat);
  const int stride = 4 * sizeof(GLfloat);
  m_program->setAttributeBuffer(0, GL_FLOAT, 0, 2, stride);
  m_program->setAttributeBuffer(1, GL_FLOAT, texOffset, 2, stride);
}


GL::RasterSymbolShader::RasterSymbolShader()
  : Shader({{QOpenGLShader::Vertex, ":chartpainter-rastersymbol.vert"},
            {QOpenGLShader::Fragment, ":chartpainter-texture.frag"}}, 3)
{
  m_locations.m_p = m_program->uniformLocation("m_p");
  m_locations.m_model = m_program->uniformLocation("m_model");
  m_locations.windowScale = m_program->uniformLocation("windowScale");
  m_locations.offset = m_program->uniformLocation("offset");
}

void GL::RasterSymbolShader::initializePaint() {
  auto f = QOpenGLContext::currentContext()->extraFunctions();
  GLint pid;
  f->glGetIntegerv(GL_CURRENT_PROGRAM, &pid);
  if (pid == static_cast<GLint>(m_program->programId())) return; // already bound
  m_program->bind();
  m_program->enableAttributeArray(0);
  m_program->enableAttributeArray(1);
  m_program->enableAttributeArray(2);
  f->glVertexAttribDivisor(0, 0);
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

  auto ds = Settings::instance()->displayLengthScaling();
  const float s = .5 * cam->heightMM() * cam->projection()(1, 1) * ds;
  m_program->setUniformValue(m_locations.windowScale, s);

  m_program->setAttributeBuffer(0, GL_FLOAT, 0, 2, 0);
}


GL::VectorSymbolShader::VectorSymbolShader()
  : Shader({{QOpenGLShader::Vertex, ":chartpainter-vectorsymbol.vert"},
            {QOpenGLShader::Fragment, ":chartpainter.frag"}}, 4)
{
  m_locations.m_p = m_program->uniformLocation("m_p");
  m_locations.m_model = m_program->uniformLocation("m_model");
  m_locations.base_color = m_program->uniformLocation("base_color");
  m_locations.windowScale = m_program->uniformLocation("windowScale");
}

void GL::VectorSymbolShader::initializePaint() {
  auto f = QOpenGLContext::currentContext()->extraFunctions();
  GLint pid;
  f->glGetIntegerv(GL_CURRENT_PROGRAM, &pid);
  if (pid == static_cast<GLint>(m_program->programId())) return; // already bound
  m_program->bind();
  m_program->enableAttributeArray(0);
  m_program->enableAttributeArray(1);
  f->glVertexAttribDivisor(0, 0);
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

  //  if (vp != vp0) {
  //    qWarning() << "bad rectangles" << vp << vp0;
  //  }

  const QPointF p1 = vp0.topLeft();
  const QPointF q1(p1.x() / va.width(), p1.y() / va.height());

  const QPointF p2 = vp0.bottomRight();
  const QPointF q2(p2.x() / va.width(), p2.y() / va.height());

  const QPointF tr = q1 + QPointF(.5, .5);
  const QPointF sc = (q2 - q1);

  const qreal v0 = 0.;
  const qreal v1 = 1.;
  const qreal s = qMax(v0, qMin(v1 - sc.x(), tr.x()));
  const qreal t = qMax(v0, qMin(v1 - sc.y(), tr.y()));

  m_program->setUniformValue(m_locations.tr_tex, QPointF(s, t));
  m_program->setUniformValue(m_locations.scale_tex, sc);

  const int texOffset = 2 * sizeof(GLfloat);
  const int stride = 4 * sizeof(GLfloat);
  m_program->setAttributeBuffer(0, GL_FLOAT, 0, 2, stride);
  m_program->setAttributeBuffer(1, GL_FLOAT, texOffset, 2, stride);
}

GL::TextureShader::TextureShader()
  : Shader({{QOpenGLShader::Vertex, ":chartpainter-texture.vert"},
            {QOpenGLShader::Fragment, ":chartpainter-texture.frag"}}, 0)
{
  m_locations.m_pv = m_program->uniformLocation("m_pv");
  m_locations.scale_tex = m_program->uniformLocation("scale_tex");
  m_locations.tr_tex = m_program->uniformLocation("tr_tex");
  m_locations.scale_vertex = m_program->uniformLocation("scale_vertex");
  m_locations.tr_vertex = m_program->uniformLocation("tr_vertex");
}

void GL::TextureShader::initializePaint() {
  auto f = QOpenGLContext::currentContext()->extraFunctions();
  GLint pid;
  f->glGetIntegerv(GL_CURRENT_PROGRAM, &pid);
  if (pid == static_cast<GLint>(m_program->programId())) return; // already bound
  m_program->bind();
  m_program->enableAttributeArray(0);
  m_program->enableAttributeArray(1);
  f->glVertexAttribDivisor(0, 0);
  f->glVertexAttribDivisor(1, 0);
}
