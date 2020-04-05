#include "globe.h"
#include <gdal/ogrsf_frmts.h>
#include "earcut.hpp"
#include <QDebug>
#include <QOpenGLContext>
#include <QOpenGLExtraFunctions>
#include <QFile>
#include <QDir>
#include <QRegularExpression>
#include <QTextStream>

Globe::Globe()
  : m_coordBuffer(QOpenGLBuffer::VertexBuffer)
  , m_indexBuffer(QOpenGLBuffer::IndexBuffer)
{

  // TODO: gsshg charts from known locations
  QDir chartDir("/home/jusirkka/share/gshhg/GSHHS_shp/c");
  QStringList charts = chartDir.entryList(QStringList() << "*_L?.shp",
                                          QDir::Files | QDir::Readable, QDir::Name);

  if (charts.isEmpty()) {
    throw ChartFileError(QString("%1 is not a valid chart directory").arg(chartDir.absolutePath()));
  }
  qDebug() << charts;
  initLayers();

  for (const QString& chart: charts) {
    auto path = chartDir.absoluteFilePath(chart);
    auto world = static_cast<GDALDataset*>(GDALOpenEx(path.toUtf8(), GDAL_OF_VECTOR, nullptr, nullptr, nullptr));
    if (world == nullptr) continue;

    OGRLayer* layer = world->GetLayer(0);
    int index = indexFromChartName(chart);
    m_layerData[index].offset = m_triangles.size();
    for (const OGRFeatureUniquePtr& feature: layer) {
      for (int i = 0; i < feature->GetGeomFieldCount(); i++) {
        triangulate(feature->GetGeomFieldRef(i));
      }
    }

    m_layerData[index].length = m_triangles.size() - m_layerData[index].offset;
    qDebug() << index << m_layerData[index].offset << m_layerData[index].length;
  }

  qDebug() << "size of mesh =" << m_coords.size();
  qDebug() << "number of triangles =" << m_triangles.size() << "/ 3 = " << m_triangles.size() / 3;
}

void Globe::initializeGL(QOpenGLWidget *context) {
  m_program = new QOpenGLShaderProgram(context);

  struct Source {
    QOpenGLShader::ShaderTypeBit stype;
    QString fname;
  };
  const QVector<Source> sources{
    {QOpenGLShader::Vertex, ":/shaders/simple.vert"},
    {QOpenGLShader::TessellationControl, ":/shaders/globe.tesc"},
    {QOpenGLShader::TessellationEvaluation, ":/shaders/globe.tese"},
    {QOpenGLShader::Geometry, ":/shaders/globe.geom"},
    {QOpenGLShader::Fragment, ":/shaders/globe.frag"},
  };

  for (const Source& s: sources) {
    QFile file(s.fname);
    if (!file.open(QIODevice::ReadOnly)) {
      qFatal("Failed to open %s for reading", s.fname.toUtf8().data());
    }
    QString src = file.readAll();
    src = src.replace("#version 320 es", "#version 450");
    if (!m_program->addCacheableShaderFromSourceCode(s.stype, src)) {
      qFatal("Failed to compile %s: %s", s.fname.toUtf8().data(), m_program->log().toUtf8().data());
    }
  }

  if (!m_program->link()) {
    qFatal("Failed to link the globe program: %s", m_program->log().toUtf8().data());
  }

  // locations
  m_program->bind();
  m_locations.point = m_program->attributeLocation("point");
  m_locations.base_color = m_program->uniformLocation("base_color");
  m_locations.m_pv = m_program->uniformLocation("m_pv");
  m_locations.lp = m_program->uniformLocation("lp");
  m_locations.eye = m_program->uniformLocation("eye");
  m_locations.layer_index = m_program->uniformLocation("layer_index");

  // buffers
  m_coordBuffer.create();
  m_coordBuffer.bind();
  m_coordBuffer.setUsagePattern(QOpenGLBuffer::StaticDraw);
  GLsizei dataLen = m_coords.size() * 2 * sizeof(GLfloat);
  m_coordBuffer.allocate(dataLen);
  GLfloat data[m_coords.size() * 2];
  int offset = 0;

  for (const QVector2D& p: m_coords) {
    data[offset] = p.x();
    data[offset + 1] = p.y();
    offset += 2;
  }
  m_coordBuffer.write(0, data, dataLen);
  // not needed anymore
  m_coords.clear();

  m_indexBuffer.create();
  m_indexBuffer.bind();
  m_indexBuffer.setUsagePattern(QOpenGLBuffer::StaticDraw);
  m_indexCount = m_triangles.size();
  GLsizei elemLen = m_indexCount * sizeof(GLuint);
  m_indexBuffer.allocate(elemLen);
  offset = 0;
  GLuint elems[m_indexCount];
  for (quint32 i: m_triangles) {
    elems[offset] = i;
    offset += 1;
  }
  m_indexBuffer.write(0, elems, elemLen);
  // not needed anymore
  m_triangles.clear();

}

void Globe::paintGL(const Camera *cam) {
  auto gl = QOpenGLContext::currentContext()->extraFunctions();

  m_program->bind();

  // data buffers
  m_program->enableAttributeArray(m_locations.point);
  m_coordBuffer.bind();
  m_program->setAttributeBuffer(m_locations.point, GL_FLOAT, 0, 2, 0);

  const QVector3D eye = cam->view().inverted().column(3).toVector3D();
  // light direction = eye direction
  const QVector3D lp = eye.normalized();

  m_program->setUniformValue(m_locations.eye, eye);
  m_program->setUniformValue(m_locations.lp, lp);

  // other uniforms
  m_program->setUniformValue(m_locations.m_pv, cam->projection() * cam->view());

  gl->glPatchParameteri(GL_PATCH_VERTICES, 3);
  m_indexBuffer.bind();

  for (int i = LAYERS - 1; i >= 0; i--) {
    auto layer = m_layerData[i];
    if (layer.length == 0) continue;
    m_program->setUniformValue(m_locations.base_color, layer.color);
    m_program->setUniformValue(m_locations.layer_index, i);
    gl->glDrawElements(GL_PATCHES, layer.length,  GL_UNSIGNED_INT,
                       reinterpret_cast<const GLvoid*>(layer.offset * sizeof(GLuint)));
  }
}

void Globe::initLayers() {
  // sea
  m_layerData[0].color = QColor("#ecf3ff");
  m_layerData[0].offset = m_triangles.size();
  triangulateSphere();
  m_layerData[0].length = m_triangles.size() - m_layerData[0].offset;

  // continents
  m_layerData[1].color = QColor("#faf1a4");
  m_layerData[1].offset = 0;
  m_layerData[1].length = 0;

  // lakes
  m_layerData[2].color = QColor("#9fc7fd");
  m_layerData[2].offset = 0;
  m_layerData[2].length = 0;

  // isles
  m_layerData[3].color = QColor("#faf1a4");
  m_layerData[3].offset = 0;
  m_layerData[3].length = 0;

  // ponds
  m_layerData[4].color = QColor("#9fc7fd");
  m_layerData[4].offset = 0;
  m_layerData[4].length = 0;

  // antarctica/ice
  m_layerData[5].color = QColor("#dddddd");
  m_layerData[5].offset = 0;
  m_layerData[5].length = 0;

  // antarctica/ground
  m_layerData[6].color = QColor("#84b496");
  m_layerData[6].offset = 0;
  m_layerData[6].length = 0;

}

void Globe::triangulateSphere() {
  int N_lng = 31;
  int N_lat = 11;
  for (int j = 0; j < N_lat; j++) {
    float lat = 60 - 10 * j;
    for (int i = 0; i < N_lng; i++) {
      float lng = -140 + 10 * i;
      m_coords.append(QVector2D(lng, lat));
    }
  }

  for (int j = 0; j < N_lat - 1; j++) {
    for (int i = 0; i < N_lng - 1; i++) {
      // triangle 1
      m_triangles.append(N_lng * j + i);
      m_triangles.append(N_lng * (j + 1) + i);
      m_triangles.append(N_lng * j + i + 1);
      // triangle 2
      m_triangles.append(N_lng * (j + 1) + i);
      m_triangles.append(N_lng * (j + 1) + i + 1);
      m_triangles.append(N_lng * j + i + 1);
    }
  }

}


int Globe::indexFromChartName(const QString &name) {
  static const QRegularExpression re(".*_L(\\d)\\.shp$");
  QRegularExpressionMatch match = re.match(name);
  if (!match.hasMatch()) {
    throw ChartFileError(QString("Error parsing chart name %1").arg(name));
  }
  return match.captured(1).toInt();
}

QString Globe::rectToText(const QRectF &clip) {
  auto c = clip.normalized();
  if (!c.isValid()) return QString();
  QString p("POLYGON((");
  QTextStream poly(&p);
  poly << c.x() << " " << c.y();
  poly << ", " << c.x() + c.width() << " " << c.y();
  poly << ", " << c.x() + c.width() << " " << c.y() + c.height();
  poly << ", " << c.x() << " " << c.y() + c.height();
  poly << ", " << c.x() << " " << c.y() << "))";
  qDebug() << p;
  return p;
}

void Globe::triangulate(const OGRGeometry* geom) {

  if (geom->IsEmpty()) return;

  const QString geomType(geom->getGeometryName());
  if (geomType == "GEOMETRYCOLLECTION") {
    auto collection = dynamic_cast<const OGRGeometryCollection*>(geom);
    for (auto item: collection) {
      triangulate(item);
    }
    return;
  }

  if (geomType == "MULTIPOLYGON") {
    auto collection = dynamic_cast<const OGRMultiPolygon*>(geom);
    for (auto item: collection) {
      triangulate(item);
    }
    return;
  }

  if (geomType != "POLYGON") {
    qWarning() << "Don't know how to triangulate" << geomType;
    return;
  }

  auto poly = dynamic_cast<const OGRPolygon*>(geom);

  // The number type to use for tessellation
  using Coord = double;

  // The index type. Defaults to uint32_t, but you can also pass uint16_t if you know that your
  // data won't have more than 65536 vertices.
  using N = quint32;

  quint32 offset = m_coords.size();

  // Create array
  using Point = std::array<Coord, 2>;
  std::vector<std::vector<Point>> polygon;


  std::vector<Point> points;
  for (OGRPoint p: poly->getExteriorRing()) {
    points.push_back(Point{p.getX(), p.getY()});
    m_coords.append(QVector2D(p.getX(), p.getY()));
  }
  polygon.push_back(points);

  for (int i = 0; i < poly->getNumInteriorRings(); i++) {
    points.clear();
    for (OGRPoint p: poly->getInteriorRing(i)) {
      points.push_back(Point{p.getX(), p.getY()});
      m_coords.append(QVector2D(p.getX(), p.getY()));
    }
    polygon.push_back(points);
  }
  // Run tessellation
  // Returns array of indices that refer to the vertices of the input polygon.
  // Three subsequent indices form a triangle. Output triangles are clockwise.
  std::vector<N> indices = mapbox::earcut<N>(polygon);

  for (N index: indices) {
    m_triangles.append(index + offset);
  }

}

OGRGeometry* Globe::createGeometry(const QString& txt) const {
  if (txt.isEmpty()) return nullptr;
  OGRGeometry* handle;
  OGRErr err = OGRGeometryFactory::createFromWkt(txt.toUtf8(), nullptr, &handle);
  if (err == OGRERR_NONE) return handle;
  qWarning() << "createGeometry failed";
  return nullptr;
}
