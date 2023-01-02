/* -*- coding: utf-8-unix -*-
 *
 * File: src/textmanager.cpp
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
#include "textmanager.h"
#include <QThread>
#include <QMutexLocker>
#include <QOpenGLTexture>
#include <QTimer>
#include "logging.h"
#include "platform.h"

TextManager::TextManager()
  : QObject()
  , m_mutex()
  , m_thread(new QThread)
  , m_worker(new TextShaper(&m_mutex))
  , m_glyphTexture(new QOpenGLTexture(QOpenGLTexture::Target2D))
  , m_shapeTimer(new QTimer(this))
{
  m_worker->moveToThread(m_thread);
  connect(m_thread, &QThread::finished, m_worker, &QObject::deleteLater);
  connect(m_worker, &TextShaper::done, this, &TextManager::handleShape);
  m_thread->start();

  m_shapeTimer->setInterval(200);
  connect(m_shapeTimer, &QTimer::timeout, this, &TextManager::requestUpdate);
}

void TextManager::init() {
  qCDebug(CTXT) << "init (noop)";
}

TextManager::~TextManager() {
  m_thread->quit();
  m_thread->wait();
  delete m_thread;
  delete m_glyphTexture;
}

void TextManager::finalizeGL() {
  m_glyphTexture->destroy();
}

void TextManager::initializeGL() {
  updateTextureData();
}

TextManager* TextManager::instance() {
  static TextManager* m = new TextManager();
  return m;
}

void TextManager::createTexture(int w, int h) {
  m_glyphTexture->destroy();
  m_glyphTexture->create();
  m_glyphTexture->bind();
  m_glyphTexture->setFormat(QOpenGLTexture::R8_UNorm);
  m_glyphTexture->setSize(w, h);
  m_glyphTexture->setWrapMode(QOpenGLTexture::ClampToEdge);
  m_glyphTexture->setMinMagFilters(QOpenGLTexture::Linear, QOpenGLTexture::Linear);
  m_glyphTexture->setMipLevels(1);
  m_glyphTexture->allocateStorage(QOpenGLTexture::Red, QOpenGLTexture::UInt8);
}

void TextManager::bind() {
  m_glyphTexture->bind();
}

int TextManager::atlasWidth() const {
  return m_glyphTexture->width();
}

int TextManager::atlasHeight() const {
  return m_glyphTexture->height();
}

void TextManager::handleShape(const TextKey& key,
                              GL::Mesh* mesh,
                              bool ng) {

  if (ng) emit newGlyphs();

  { // scope for mutex locker
    QMutexLocker lock(&m_mutex);

    m_shapeData[m_tickets[key]] = mesh->vertices;
    delete mesh;
  }

  m_shapeTimer->start();
}


void TextManager::updateTextureData() {
  auto atlas = m_worker->atlas();
  QMutexLocker lock(&m_mutex);

  if (!m_glyphTexture->isCreated() ||
      m_glyphTexture->width() != atlas.width || m_glyphTexture->height() != atlas.height) {
    createTexture(atlas.width, atlas.height);
  }

  // update texture buffer
  m_glyphTexture->bind();
  m_glyphTexture->setData(QOpenGLTexture::Red, QOpenGLTexture::UInt8, atlas.data);
}

int TextManager::ticket(const QString& txt, TXT::Weight weight,
                        TXT::HJust hjust, TXT::VJust vjust,
                        quint8 bodySize, qint8 offsetX, qint8 offsetY) {

  const TextKey key(txt, weight, hjust, vjust, bodySize, offsetX, offsetY);

  int index = 0;
  { // scope for mutex locker
    QMutexLocker lock(&m_mutex);

    if (m_tickets.contains(key)) return m_tickets[key];

    index = m_shapeData.size();
    // reserve space for shapedata
    m_tickets[key] = index;
    m_shapeData << GL::VertexVector();
  }
  QMetaObject::invokeMethod(m_worker, "shape",
                            Q_ARG(const TextKey&, key));

  return index;
}

void TextManager::requestUpdate() {
  m_shapeTimer->stop();
  emit newStrings();
}


TextShaper::TextShaper(QMutex* mutex)
  : QObject()
  , m_manager(mutex) {}

GL::GlyphData TextShaper::atlas() const {
  GL::GlyphData ret {m_manager.width(), m_manager.height(), m_manager.data()};
  return ret;
}

void TextShaper::shape(const TextKey &key) {
  m_manager.setFont(key.weight);
  bool newGlyphs;
  GL::Mesh* mesh = m_manager.shapeText(HB::Text(key.text), &newGlyphs);

  // apply transformations

  const float bodySizeMM = key.bodySize * .351 * Platform::display_text_size_scaling();
  const auto dPivot = QPointF(m_pivotHMap[key.hjust] * mesh->bbox.width(),
                              m_pivotVMap[key.vjust] * mesh->bbox.height());

  const auto boxOffsetMM = QPointF(key.offsetX, - key.offsetY) * bodySizeMM;
  const auto boxScale = bodySizeMM / mesh->bbox.height();
  const auto shift = boxOffsetMM + boxScale * (dPivot - mesh->bbox.bottomLeft()); // invertex y-axis

  const int numShapes = mesh->vertices.size() / 10;
  for (int i = 0; i < numShapes; i++) {
    const auto x0 = boxScale * mesh->vertices[10 * i + 0];
    const auto y0 = boxScale * mesh->vertices[10 * i + 1];
    const auto x1 = boxScale * mesh->vertices[10 * i + 2];
    const auto y1 = boxScale * mesh->vertices[10 * i + 3];

    mesh->vertices[10 * i + 0] = x0 + shift.x();
    mesh->vertices[10 * i + 1] = y0 + shift.y();
    mesh->vertices[10 * i + 2] = x1 + shift.x();
    mesh->vertices[10 * i + 3] = y1 + shift.y();
  }
  emit done(key, mesh, newGlyphs);
}
