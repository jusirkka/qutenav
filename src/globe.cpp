#include "globe.h"
#include <QDebug>
#include <QOpenGLContext>
#include <QOpenGLExtraFunctions>
#include <QFile>
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
{

  QStringList charts;
  QDir chartDir;
  QStringList locs;

  for (const QString& loc: QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation)) {
    locs << QString("%1/qopencpn/GSHHS/c").arg(loc);
  }

  for (const QString& loc: locs) {
    chartDir = QDir(loc);
    charts = chartDir.entryList(QStringList() << "globe_l?.obj",
                                QDir::Files | QDir::Readable, QDir::Name);
    if (!charts.isEmpty()) break;
  }

  if (charts.isEmpty()) {
    throw ChartFileError(QString("Could not find a valid chart directory in %1").arg(locs.join(", ")));
  }

  initLayers();


  WFReader p;
  size_t elemSize = 0;
  GLsizei offset = 0;
  for (const QString& chart: charts) {
    auto path = chartDir.absoluteFilePath(chart);
    p.parse(path);

    m_coords.append(p.vertices());

    int i = indexFromChartName(chart);


    m_layerData[i].offset = offset;

    int stripSize = 0;
    for (const Indices& strip: p.strips()) {
      StripData s;
      s.offset = elemSize + stripSize;
      s.size = strip.size();
      m_layerData[i].strips.append(s);
      stripSize += strip.size() * sizeof(GLuint);
      m_strips.append(strip);
    }

    elemSize += stripSize;
    offset += p.vertices().size() * sizeof(GLfloat);

    p.reset();
  }
}

void Globe::initializeGL() {
  m_program = new QOpenGLShaderProgram(this);

  struct Source {
    QOpenGLShader::ShaderTypeBit stype;
    QString fname;
  };

  const QVector<Source> sources{
    {QOpenGLShader::Vertex, ":/shaders/globe.vert"},
    {QOpenGLShader::Fragment, ":/shaders/globe.frag"},
  };

  for (const Source& s: sources) {
    if (!m_program->addCacheableShaderFromSourceFile(s.stype, s.fname)) {
      qFatal("Failed to compile %s: %s", s.fname.toUtf8().data(), m_program->log().toUtf8().data());
    }
  }

  if (!m_program->link()) {
    qFatal("Failed to link the globe program: %s", m_program->log().toUtf8().data());
  }

  // buffers
  m_coordBuffer.create();
  m_coordBuffer.bind();
  m_coordBuffer.setUsagePattern(QOpenGLBuffer::StaticDraw);
  GLsizei dataLen = m_coords.size() * sizeof(GLfloat);
  m_coordBuffer.allocate(dataLen);

  m_coordBuffer.write(0, m_coords.constData(), dataLen);

  // not needed anymore
  m_coords.clear();

  m_indexBuffer.create();
  m_indexBuffer.bind();
  m_indexBuffer.setUsagePattern(QOpenGLBuffer::StaticDraw);
  GLsizei elemLen = m_strips.size() * sizeof(GLuint);
  m_indexBuffer.allocate(elemLen);
  m_indexBuffer.write(0, m_strips.constData(), elemLen);
  // not needed anymore
  m_strips.clear();

  // locations
  m_program->bind();
  m_locations.base_color = m_program->uniformLocation("base_color");
  m_locations.m_pv = m_program->uniformLocation("m_pv");
  m_locations.lp = m_program->uniformLocation("lp");

  auto gl = QOpenGLContext::currentContext()->functions();
  gl->glEnable(GL_DEPTH_TEST);
  gl->glEnable(GL_STENCIL_TEST);
  gl->glEnable(GL_CULL_FACE);
  gl->glFrontFace(GL_CCW);
  gl->glCullFace(GL_BACK);
  gl->glStencilFuncSeparate(GL_FRONT, GL_EQUAL, 0, 0xff);
  gl->glStencilOpSeparate(GL_FRONT, GL_KEEP, GL_KEEP, GL_INCR);
}

void Globe::paintGL(const Camera *cam) {
  auto gl = QOpenGLContext::currentContext()->extraFunctions();

  m_program->bind();

  m_program->enableAttributeArray(0);
  m_coordBuffer.bind();
  m_indexBuffer.bind();

  const QVector3D eye = cam->view().inverted().column(3).toVector3D();
  // light direction = eye direction
  m_program->setUniformValue(m_locations.lp, eye.normalized());
  m_program->setUniformValue(m_locations.m_pv, cam->projection() * cam->view());

  for (int i = LAYERS - 1; i >= 0; i--) {
    auto layer = m_layerData[i];
    if (layer.strips.isEmpty()) continue;
    m_program->setUniformValue(m_locations.base_color, layer.color);
    m_program->setAttributeBuffer(0, GL_FLOAT, layer.offset, 3, 0);
    // FIXME: use multidrawelements
    for (const StripData& s: layer.strips) {
      gl->glDrawElements(GL_TRIANGLE_STRIP, s.size, GL_UNSIGNED_INT,
                         reinterpret_cast<const void*>(s.offset));
    }
  }
}


void Globe::initLayers() {
  // sea
  m_layerData[0].color = QColor("#d4eaee");
  m_layerData[0].offset = 0;
  m_layerData[0].strips = Strips();
  // continents
  m_layerData[1].color = QColor("#faf1a4");
  m_layerData[1].offset = 0;
  m_layerData[1].strips = Strips();
  // lakes
  m_layerData[2].color = QColor("#9fc7fd");
  m_layerData[2].offset = 0;
  m_layerData[2].strips = Strips();
  // isles
  m_layerData[3].color = QColor("#faf1a4");
  m_layerData[3].offset = 0;
  m_layerData[3].strips = Strips();
  // ponds
  m_layerData[4].color = QColor("#9fc7fd");
  m_layerData[4].offset = 0;
  m_layerData[4].strips = Strips();
  // antarctica/ice
  m_layerData[5].color = QColor("#dddddd");
  m_layerData[5].offset = 0;
  m_layerData[5].strips = Strips();
  // antarctica/ground
  m_layerData[6].color = QColor("#84b496");
  m_layerData[6].offset = 0;
  m_layerData[6].strips = Strips();
}


int Globe::indexFromChartName(const QString &name) {
  static const QRegularExpression re(".*_l(\\d)\\.obj$");
  QRegularExpressionMatch match = re.match(name);
  if (!match.hasMatch()) {
    throw ChartFileError(QString("Error parsing chart name %1").arg(name));
  }
  return match.captured(1).toInt();
}

