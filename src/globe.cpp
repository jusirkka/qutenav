#include "globe.h"
#include <QDebug>
#include <QOpenGLContext>
#include <QOpenGLExtraFunctions>
#include <QOpenGLTexture>
#include <QDir>
#include <QRegularExpression>
#include <QTextStream>
#include <QStandardPaths>
#include "wfreader.h"
#include "types.h"

Globe::Globe(QObject *parent)
  : Drawable(parent)
  , m_coordBuffer(QOpenGLBuffer::VertexBuffer)
  , m_indexBuffer(QOpenGLBuffer::IndexBuffer)
  , m_program(nullptr)
  , m_globeTexture(new QOpenGLTexture(QOpenGLTexture::TargetCubeMap))
{}

Globe::~Globe() {
  delete m_program;
  delete m_globeTexture;
}

void Globe::initializeGL() {
  if (m_program != nullptr) return;

  m_program = new QOpenGLShaderProgram;

  struct Source {
    QOpenGLShader::ShaderTypeBit stype;
    QString fname;
  };

  const QVector<Source> sources{
    {QOpenGLShader::Vertex, ":/shaders/globe.vert"},
    {QOpenGLShader::Fragment, ":/shaders/globe.frag"},
  };

  for (const Source& s: sources) {
    if (!m_program->addShaderFromSourceFile(s.stype, s.fname)) {
      qFatal("Failed to compile %s: %s", s.fname.toUtf8().data(), m_program->log().toUtf8().data());
    }
  }

  if (!m_program->link()) {
    qFatal("Failed to link the globe program: %s", m_program->log().toUtf8().data());
  }

  // locations
  m_program->bind();
  m_locations.globe = m_program->uniformLocation("globe");
  m_locations.m_pv = m_program->uniformLocation("m_pv");
  m_locations.lp = m_program->uniformLocation("lp");

  QStringList objectFiles;
  QDir globeDir;
  QStringList locs;

  for (const QString& loc: QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation)) {
    locs << QString("%1/%2/globe").arg(loc).arg(qAppName());
  }

  for (const QString& loc: locs) {
    globeDir = QDir(loc);
    objectFiles = globeDir.entryList(QStringList() << "sphere.obj",
                                     QDir::Files | QDir::Readable);
    if (!objectFiles.isEmpty()) break;
  }

  if (objectFiles.isEmpty()) {
    throw ChartFileError(QString("Could not find a valid globe directory in %1").arg(locs.join(", ")));
  }

  WFReader p;

  auto path = globeDir.absoluteFilePath(objectFiles.first());
  p.parse(path);

  // copy vertices/indices to buffers
  m_coordBuffer.create();
  m_coordBuffer.bind();
  m_coordBuffer.setUsagePattern(QOpenGLBuffer::StaticDraw);
  GLsizei dataLen = p.vertices().size() * sizeof(GLfloat);
  m_coordBuffer.allocate(p.vertices().constData(), dataLen);

  m_indexBuffer.create();
  m_indexBuffer.bind();
  m_indexBuffer.setUsagePattern(QOpenGLBuffer::StaticDraw);
  GLsizei elemLen = p.triangles().size() * sizeof(GLuint);
  m_indexBuffer.allocate(p.triangles().constData(), elemLen);

  m_indexCount = p.triangles().size();

  const int stride = 3 * sizeof(GLfloat);
  m_program->setAttributeBuffer(0, GL_FLOAT, 0, 3, stride);

  m_program->enableAttributeArray(0);

  // cubemap
  loadImages(globeDir);
}

void Globe::updateCharts(const Camera* /*cam*/, const QRectF& /*viewArea*/)  {
 // noop
}

void Globe::paintGL(const Camera *cam) {
  auto f = QOpenGLContext::currentContext()->functions();
  f->glDisable(GL_BLEND);
  f->glEnable(GL_DEPTH_TEST);
  f->glEnable(GL_STENCIL_TEST);
  f->glEnable(GL_CULL_FACE);
  f->glFrontFace(GL_CW);
  f->glCullFace(GL_BACK);
  f->glStencilFunc(GL_EQUAL, 0, 0xff);
  f->glStencilOp(GL_KEEP, GL_KEEP, GL_INCR);
  f->glStencilMask(0xff);
  m_program->bind();

  m_program->enableAttributeArray(0);
  m_coordBuffer.bind();
  m_indexBuffer.bind();

  m_globeTexture->bind();

  m_program->setUniformValue(m_locations.globe, 0);
  const QVector3D eye = cam->view().inverted().column(3).toVector3D();
  // light direction = eye direction
  m_program->setUniformValue(m_locations.lp, eye.normalized());
  m_program->setUniformValue(m_locations.m_pv, cam->projection() * cam->view());

  f->glDrawElements(GL_TRIANGLES, m_indexCount, GL_UNSIGNED_INT, nullptr);

}

void Globe::loadImages(const QDir& imageDir) {
  QVector<QImage> images;
  // nex, nexy, nexz, posx, posy, posz
  auto imageFiles = imageDir.entryList(QStringList() << "*.jpg",
                                       QDir::Files | QDir::Readable, QDir::Name);
  qDebug() << imageFiles;

  for (const QString& imageFile: imageFiles) {
    auto image = QImage(imageDir.absoluteFilePath(imageFile));
    images << image.convertToFormat(QImage::Format_RGBA8888);
  }

  m_globeTexture->create();
  m_globeTexture->bind();

  m_globeTexture->setSize(images.first().width(), images.first().height(), images.first().depth());

  m_globeTexture->setFormat(QOpenGLTexture::RGBA8_UNorm);
  m_globeTexture->allocateStorage();

  QVector<QOpenGLTexture::CubeMapFace> faces {
    QOpenGLTexture::CubeMapNegativeX,
    QOpenGLTexture::CubeMapNegativeY,
    QOpenGLTexture::CubeMapNegativeZ,
    QOpenGLTexture::CubeMapPositiveX,
    QOpenGLTexture::CubeMapPositiveY,
    QOpenGLTexture::CubeMapPositiveZ,
  };

  for (int i = 0; i < images.size(); i++) {
    m_globeTexture->setData(0, 0, faces[i],
                            QOpenGLTexture::RGBA, QOpenGLTexture::UInt8,
                            images[i].constBits(), nullptr);
  }

  m_globeTexture->setWrapMode(QOpenGLTexture::ClampToEdge);
  m_globeTexture->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
  m_globeTexture->setMagnificationFilter(QOpenGLTexture::Linear);

}

void Globe::loadImage(const QDir& imageDir) {
  auto imageFiles = imageDir.entryList(QStringList() << "texture.png",
                                       QDir::Files | QDir::Readable, QDir::Name);
  qDebug() << imageFiles;

  auto image = QImage(imageDir.absoluteFilePath(imageFiles.first()))
      .convertToFormat(QImage::Format_RGBA8888);

  m_globeTexture->create();
  m_globeTexture->bind();

  const int w = image.width() / 4;
  const int h = image.height() / 3;
  m_globeTexture->setSize(w, h, image.depth());

  m_globeTexture->setFormat(QOpenGLTexture::RGBA8_UNorm);
  m_globeTexture->allocateStorage();

  struct CubeData {
    QOpenGLTexture::CubeMapFace face;
    int xOffset;
    int yOffset;
  };

  QVector<CubeData> faces {
    {QOpenGLTexture::CubeMapNegativeX, 0, h}, // left
    {QOpenGLTexture::CubeMapPositiveX, 2 * w, h}, // right
    {QOpenGLTexture::CubeMapNegativeY, w, 2 * h}, // bottom
    {QOpenGLTexture::CubeMapPositiveY, w, 0}, // top
    {QOpenGLTexture::CubeMapNegativeZ, 3 * w, h}, // back
    {QOpenGLTexture::CubeMapPositiveZ, w, h}, // front
  };

  for (const CubeData face: faces) {
    auto subimage = image.copy(face.xOffset, face.yOffset, w, h);
    m_globeTexture->setData(0, 0, face.face,
                            QOpenGLTexture::RGBA, QOpenGLTexture::UInt8,
                            subimage.constBits(), nullptr);
  }

  m_globeTexture->setWrapMode(QOpenGLTexture::ClampToEdge);
  m_globeTexture->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
  m_globeTexture->setMagnificationFilter(QOpenGLTexture::Linear);

}

