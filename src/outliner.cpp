/* -*- coding: utf-8-unix -*-
 *
 * File: src/outliner.cpp
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
  , m_vertexBuffer(QOpenGLBuffer::VertexBuffer)
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
      {QOpenGLShader::Vertex, ":outliner.vert"},
      {QOpenGLShader::Fragment, ":outliner.frag"},
    };

    for (const Source& s: sources) {
      if (!m_program->addShaderFromSourceFile(s.stype, s.fname)) {
        qFatal("Failed to compile %s: %s", s.fname.toUtf8().data(), m_program->log().toUtf8().data());
      }
    }

    if (!m_program->link()) {
      qFatal("Failed to link the outliner program: %s", m_program->log().toUtf8().data());
    }

    m_vertexBuffer.create();
    m_vertexBuffer.setUsagePattern(QOpenGLBuffer::DynamicDraw);

    // locations
    m_program->bind();
    m_locations.base_color = m_program->uniformLocation("base_color");
    m_locations.m_pv = m_program->uniformLocation("m_pv");

  }

  updateBuffers();
}

void Outliner::updateCharts(const Camera* /*cam*/, const QRectF& /*viewArea*/) {
  // noop
}

void Outliner::paintGL(const Camera* cam) {

  m_vertexBuffer.bind();

  m_program->bind();
  m_program->enableAttributeArray(0);
  m_program->setAttributeBuffer(0, GL_FLOAT, 0, 3, 0);

  auto f = QOpenGLContext::currentContext()->extraFunctions();

  f->glEnable(GL_DEPTH_TEST);
  f->glDisable(GL_BLEND);
  f->glDisable(GL_STENCIL_TEST);
  //  f->glDisable(GL_CULL_FACE);
  //  f->glFrontFace(GL_CW);
  //  f->glCullFace(GL_BACK);

  // uniforms
  m_program->setUniformValue(m_locations.m_pv, cam->projection() * cam->view());
  m_program->setUniformValue(m_locations.base_color, QColor("#00ff00"));

  int first = 0;
  for (auto s: m_lineStringSizes) {
    f->glDrawArrays(GL_LINE_STRIP, first, s);
    first += s;
  }
}

void Outliner::writeVertex(GL::VertexVector& vertices, const WGS84Point& p) const {
  const float r = 1.002;
  const double lng = p.radiansLng();
  const double lat = p.radiansLat();
  vertices << r * cos(lng) * cos(lat);
  vertices << r * sin(lng) * cos(lat);
  vertices << r * sin(lat);
}


void Outliner::updateBuffers() {

  m_lineStringSizes.clear();


  GL::VertexVector vertices;

  const WGS84Polygon& polys = m_manager->outlines();
  const qreal D = 1.e5; // 100 km
  for (const WGS84PointVector& ps: polys) {
    const int N = ps.size();
    int lineStringSize = 0;
    for (int i = 0; i < N - 1; ++i) {
      const WGS84Bearing b = ps[i + 1] - ps[i];
      const auto K = static_cast<int>(std::ceil(b.meters() / D));
      writeVertex(vertices, ps[i]);
      for (int k = 1; k < K; ++k) {
        const double s = (static_cast<double>(k) / K);
        writeVertex(vertices, ps[i] + s * b);
      }
      lineStringSize += K;
    }
    writeVertex(vertices, ps.last());
    lineStringSize += 1;
    m_lineStringSizes.append(lineStringSize);
  }

  m_vertexBuffer.bind();
  GLsizei dataLen = vertices.size() * sizeof(GLfloat);
  m_vertexBuffer.allocate(dataLen);
  m_vertexBuffer.write(0, vertices.constData(), dataLen);

  qDebug() << "number of lines =" << vertices.size() / 3;
}
