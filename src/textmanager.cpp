#include "textmanager.h"
#include <QOpenGLContext>
#include <QOffscreenSurface>
#include <QThread>
#include <QMutexLocker>
#include <QOpenGLTexture>
#include <QTimer>
#include <QDebug>
#include "glcontext.h"

TextManager::TextManager()
  : QObject()
  , m_mutex()
  , m_thread(new QThread)
  , m_worker(new TextShaper(&m_mutex))
  , m_invalid()
  , m_coordBuffer(QOpenGLBuffer::VertexBuffer)
  , m_indexBuffer(QOpenGLBuffer::IndexBuffer)
  , m_glyphTexture(new QOpenGLTexture(QOpenGLTexture::Target2D))
  , m_vertexOffset(0)
  , m_indexOffset(0)
  , m_shapeTimer(new QTimer(this))
{
  m_worker->moveToThread(m_thread);
  connect(m_thread, &QThread::finished, m_worker, &QObject::deleteLater);
  connect(m_worker, &TextShaper::done, this, &TextManager::handleShape);
  m_thread->start();
  m_shapeTimer->setInterval(300);
  connect(m_shapeTimer, &QTimer::timeout, this, &TextManager::requestUpdate);
}

TextManager::~TextManager() {
  m_thread->quit();
  m_thread->wait();
  delete m_thread;
  GL::Context::instance()->makeCurrent();
  delete m_glyphTexture;
}

TextManager* TextManager::instance() {
  static TextManager* m = new TextManager();
  return m;
}

void TextManager::createBuffers() {

  GL::Context::instance()->makeCurrent();

  // initialize vertex buffer
  m_coordBuffer.create();
  m_coordBuffer.bind();
  m_coordBuffer.setUsagePattern(QOpenGLBuffer::DynamicDraw);
  // allocate space for 3K characters
  m_coordBuffer.allocate(3000 * 4 * 4 * sizeof(GLfloat));

  // initialize index buffer
  m_indexBuffer.create();
  m_indexBuffer.bind();
  m_indexBuffer.setUsagePattern(QOpenGLBuffer::DynamicDraw);
  // allocate space for 3K characters
  m_indexBuffer.allocate(3000 * 6 * sizeof(GLuint));

  m_glyphTexture->setFormat(QOpenGLTexture::R8_UNorm);
  m_glyphTexture->setSize(128, 128);

  m_glyphTexture->setWrapMode(QOpenGLTexture::ClampToEdge);
  m_glyphTexture->setMinMagFilters(QOpenGLTexture::Linear, QOpenGLTexture::Linear);

  m_glyphTexture->allocateStorage(QOpenGLTexture::Red, QOpenGLTexture::UInt8);

}

void TextManager::bind() {
  m_indexBuffer.bind();
  m_coordBuffer.bind();
  m_glyphTexture->bind();
}

int TextManager::atlasWidth() const {
  return m_glyphTexture->width();
}

int TextManager::atlasHeight() const {
  return m_glyphTexture->height();
}

void TextManager::requestUpdate() {
  m_shapeTimer->stop();
  emit newStrings();
}

void TextManager::handleShape(const TextKey& key, GL::Mesh* mesh, const GL::GlyphData& atlas) {

  if (m_textMap.contains(key)) {
    delete mesh;
    return;
  }

  m_shapeTimer->start();

  GL::Context::instance()->makeCurrent();

  if (atlas.newGlyphs) {
    QMutexLocker lock(&m_mutex);

    // update texture buffer
    m_glyphTexture->bind();

    if (m_glyphTexture->width() != atlas.width || m_glyphTexture->height() != atlas.height) {
      m_glyphTexture->destroy();
      m_glyphTexture->create();
      m_glyphTexture->bind();
      m_glyphTexture->setFormat(QOpenGLTexture::R8_UNorm);
      m_glyphTexture->setSize(atlas.width, atlas.height);
      m_glyphTexture->setWrapMode(QOpenGLTexture::ClampToEdge);
      m_glyphTexture->setMinMagFilters(QOpenGLTexture::Linear, QOpenGLTexture::Linear);
      m_glyphTexture->allocateStorage(QOpenGLTexture::Red, QOpenGLTexture::UInt8);
    }

    m_glyphTexture->setData(QOpenGLTexture::Red, QOpenGLTexture::UInt8, atlas.data);
  }

  // update vertex buffer
  m_coordBuffer.bind();
  const GLsizei vLen = m_vertexOffset + sizeof(GLfloat) * mesh->vertices.size();
  if (vLen > m_coordBuffer.size()) {
    auto tmp = new uchar[m_vertexOffset];
    auto coords = m_coordBuffer.map(QOpenGLBuffer::ReadOnly);
    memcpy(tmp, coords, m_vertexOffset);
    m_coordBuffer.unmap();
    // add 10% extra
    m_coordBuffer.allocate(vLen + vLen / 10);
    m_coordBuffer.write(0, tmp, m_vertexOffset);
    delete [] tmp;
  }
  m_coordBuffer.write(m_vertexOffset, mesh->vertices.constData(), vLen - m_vertexOffset);

  const uintptr_t vOff = m_vertexOffset;
  m_vertexOffset = vLen;

  // update index buffer
  m_indexBuffer.bind();
  const GLsizei iLen = m_indexOffset + sizeof(GLuint) * mesh->indices.size();
  if (iLen > m_indexBuffer.size()) {
    auto tmp = new uchar[m_indexOffset];
    auto coords = m_indexBuffer.map(QOpenGLBuffer::ReadOnly);
    memcpy(tmp, coords, m_indexOffset);
    m_indexBuffer.unmap();
    // add 10% extra
    m_indexBuffer.allocate(iLen + iLen / 10);
    m_indexBuffer.write(0, tmp, m_indexOffset);
    delete [] tmp;
  }
  m_indexBuffer.write(m_indexOffset, mesh->indices.constData(), iLen - m_indexOffset);

  const uintptr_t iOff = m_indexOffset;
  m_indexOffset = iLen;

  const S57::ElementData elems{GL_TRIANGLES, iOff, static_cast<size_t>(mesh->indices.size())};
  // qDebug() << "Elems =" << elems.mode << elems.offset << elems.count;
  m_textMap.insert(key, TextData(mesh->bbox, elems, vOff));

  delete mesh;

}

TextData TextManager::textData(const QString& txt, TXT::Weight weight) const {
  const TextKey key(txt, weight);
  if (m_textMap.contains(key)) return m_textMap[key];

  QMetaObject::invokeMethod(m_worker, [this, key] () {
    m_worker->shape(key);
  });

  return m_invalid;
}


TextShaper::TextShaper(QMutex* mutex)
  : QObject()
  , m_manager(mutex) {}


void TextShaper::shape(const TextKey &key) {
  m_manager.setFont(key.weight);
  bool newGlyphs;
  GL::Mesh* mesh = m_manager.shapeText(HB::Text(key.text), &newGlyphs);
  const GL::GlyphData atlas{newGlyphs, m_manager.width(), m_manager.height(), m_manager.data()};
  emit done(key, mesh, atlas);
}
