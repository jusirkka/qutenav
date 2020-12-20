#include "s57chart.h"
#include <QFile>
#include <QDataStream>
#include <QVector>
#include <functional>
#include "geoprojection.h"
#include "s57object.h"
#include "s52presentation.h"
#include "drawable.h"
#include <QDate>
#include "settings.h"
#include "shader.h"
#include <QOpenGLExtraFunctions>
#include <QOpenGLContext>
#include "textmanager.h"
#include "rastersymbolmanager.h"
#include "platform.h"

using Buffer = QVector<char>;
using HandlerFunc = std::function<bool (const Buffer&)>;

struct Handler {
  Handler(HandlerFunc f): func(std::move(f)) {}
  HandlerFunc func;
  ~Handler() = default;
};

S57ChartOutline::S57ChartOutline(const QString& path) {

  QFile file(path);
  if (!file.open(QFile::ReadOnly)) {
    throw ChartFileError(QString("Cannot open %1 for reading").arg(path));
  }
  QDataStream stream(&file);

  Buffer buffer;

  buffer.resize(sizeof(OSENC_Record_Base));
  if (stream.readRawData(buffer.data(), buffer.size()) == -1) {
    throw ChartFileError(QString("Error reading %1 bytes from %2").arg(buffer.size()).arg(path));
  }

  //  For identification purposes, the very first record must be the OSENC Version Number Record
  auto record = reinterpret_cast<OSENC_Record_Base*>(buffer.data());

  // Check Record
  if (record->record_type != SencRecordType::HEADER_SENC_VERSION){
    throw ChartFileError(QString("%1 is not a supported senc file").arg(path));
  }

  //  This is the correct record type (OSENC Version Number Record), so read it
  buffer.resize(record->record_length - sizeof(OSENC_Record_Base));
  if (stream.readRawData(buffer.data(), buffer.size()) == -1) {
    throw ChartFileError(QString("Error reading %1 bytes from %2").arg(buffer.size()).arg(path));
  }
  // auto p16 = reinterpret_cast<quint16*>(buffer.data());
  // qDebug() << "senc version =" << *p16;


  const QMap<SencRecordType, Handler*> handlers {
    {SencRecordType::HEADER_CELL_NAME, new Handler([] (const Buffer& b) {
        // qDebug() << "cell name" << QString::fromUtf8(b.constData());
        return true;
      })
    },
    {SencRecordType::HEADER_CELL_PUBLISHDATE, new Handler([this] (const Buffer& b) {
        auto s = QString::fromUtf8(b.constData());
        // qDebug() << "cell publishdate" << s;
        m_pub = QDate::fromString(s, "yyyyMMdd");
        return true;
      })
    },
    {SencRecordType::HEADER_CELL_UPDATEDATE, new Handler([this] (const Buffer& b) {
        auto s = QString::fromUtf8(b.constData());
        // qDebug() << "cell modified date" << s;
        m_mod = QDate::fromString(s, "yyyyMMdd");
        return true;
      })
    },
    {SencRecordType::HEADER_CELL_EDITION, new Handler([] (const Buffer& b) {
        // const quint16* p16 = reinterpret_cast<const quint16*>(b.constData());
        // qDebug() << "cell edition" << *p16;
        return true;
      })
    },
    {SencRecordType::HEADER_CELL_UPDATE, new Handler([] (const Buffer& b) {
        // const quint16* p16 = reinterpret_cast<const quint16*>(b.constData());
        // qDebug() << "cell update" << *p16;
        return true;
      })
    },
    {SencRecordType::HEADER_CELL_NATIVESCALE, new Handler([this] (const Buffer& b) {
        const quint32* p32 = reinterpret_cast<const quint32*>(b.constData());
        // qDebug() << "cell nativescale" << *p32;
        m_scale = *p32;
        return true;
      })
    },
    {SencRecordType::HEADER_CELL_SENCCREATEDATE, new Handler([] (const Buffer& b) {
        // qDebug() << "cell senccreatedate" << QString::fromUtf8(b.constData());
        return true;
      })
    },
    {SencRecordType::CELL_EXTENT_RECORD, new Handler([this] (const Buffer& b) {
        // qDebug() << "cell extent record";
        const OSENC_EXTENT_Record_Payload* p = reinterpret_cast<const OSENC_EXTENT_Record_Payload*>(b.constData());
        Extent::EightFloater points;
        points.append(p->extent_sw_lon);
        points.append(p->extent_sw_lat);
        points.append(p->extent_se_lon);
        points.append(p->extent_se_lat);
        points.append(p->extent_ne_lon);
        points.append(p->extent_ne_lat);
        points.append(p->extent_nw_lon);
        points.append(p->extent_nw_lat);

        m_extent = Extent(points);

        // reference coordinate is the mean of the extents
        m_ref = WGS84Point::fromLL(.5 * (p->extent_nw_lon + p->extent_se_lon),
                                   .5 * (p->extent_nw_lat + p->extent_se_lat));
        return true;
      })
    }
  };

  bool done = false;

  while (!done) {

    buffer.resize(sizeof(OSENC_Record_Base));
    if (stream.readRawData(buffer.data(), buffer.size()) == -1) {
      done = true;
      continue;
    }
    record = reinterpret_cast<OSENC_Record_Base*>(buffer.data());
    // copy, record_type will be overwritten in the next stream.readRawData
    SencRecordType rec_type = record->record_type;
    if (!handlers.contains(rec_type)) {
      done = true;
      continue;
    }
    buffer.resize(record->record_length - sizeof(OSENC_Record_Base));
    if (stream.readRawData(buffer.data(), buffer.size()) == -1) {
      done = true;
      continue;
    }

    done = !(handlers[rec_type])->func(buffer);

  }
  qDeleteAll(handlers);
}


//
// S57Chart
//

namespace S57 {

// Helper class to set Object's private data
class ObjectBuilder {
public:
  void addAttribute(S57::Object* obj, quint16 acode, const Attribute& a) const {
    obj->m_attributes[acode] = a;
  }
  void addString(S57::Object* obj, quint16 acode, const QString& s) const {
    if (S52::GetAttributeType(acode) == S57::Attribute::Type::IntegerList) {
      const QStringList tokens = s.split(",");
      QVariantList vs;
      for (const QString& t: tokens) {
        vs << QVariant::fromValue(t.toInt());
      }
      // qDebug() << S52::GetAttributeName(acode) << vs;
      obj->m_attributes[acode] = S57::Attribute(vs);
    } else {
      obj->m_attributes[acode] = S57::Attribute(s);
    }
  }
  bool setGeometry(S57::Object* obj, S57::Geometry::Base* g, const QRectF& bb) const {
    if (obj->m_geometry != nullptr) {
      return false;
    }
    obj->m_geometry = g;
    obj->m_bbox = bb;
    return true;
  }
};

}

static GLsizei addIndices(GLuint first, GLuint mid1, GLuint mid2, bool reversed, GL::IndexVector& indices);
static GLuint addAdjacent(GLuint base, GLuint next, GL::VertexVector& vertices);
static QRectF computeBBox(const S57::ElementDataVector &elems,
                          const GL::VertexVector& vertices,
                          const GL::IndexVector& indices);
static QPointF computeLineCenter(const S57::ElementDataVector &elems,
                                 const GL::VertexVector& vertices,
                                 const GL::IndexVector& indices);
static QPointF computeAreaCenter(const S57::ElementDataVector &elems,
                                 const GL::VertexVector& vertices, GLsizei offset);

S57Chart::S57Chart(quint32 id, const QString& path)
  : QObject()
  , m_nativeProj(new SimpleMercator)
  , m_paintData(S52::Lookup::PriorityCount)
  , m_updatedPaintData(S52::Lookup::PriorityCount)
  , m_id(id)
  , m_settings(Settings::instance())
  , m_coordBuffer(QOpenGLBuffer::VertexBuffer)
  , m_indexBuffer(QOpenGLBuffer::IndexBuffer)
  , m_pivotBuffer(QOpenGLBuffer::VertexBuffer)
{
  S57ChartOutline outline(path);
  m_extent = outline.extent();
  m_nativeProj->setReference(outline.reference());

  //  if (*proj != *m_nativeProj) {
  //    throw NotImplementedError("Only SimpleMercator supported for now");
  //  }

  QFile file(path);
  file.open(QFile::ReadOnly);
  QDataStream stream(&file);

  S57::Object* current = nullptr;
  S57::ObjectBuilder helper;

  struct TrianglePatch {
    GLenum mode;
    GL::VertexVector vertices;
  };

  using LineHandle = QVector<int>;
  using TriangleHandle = QVector<TrianglePatch>;

  struct OData {
    OData(S57::Object* obj): object(obj), lines(), triangles(), type(S57::Geometry::Type::Meta) {}
    S57::Object* object;
    LineHandle lines;
    TriangleHandle triangles;
    S57::Geometry::Type type;
  };

  QVector<OData> objects;

  struct EdgeExtent {
    int first;
    int last;
  };

  QMap<int, int> connMap;
  QMap<int, EdgeExtent> edgeMap;

  GL::VertexVector vertices;
  GL::IndexVector indices;


  const QMap<SencRecordType, Handler*> handlers {

    {SencRecordType::FEATURE_ID_RECORD, new Handler([&current, &objects, this] (const Buffer& b) {
        // qDebug() << "feature id record";
        auto p = reinterpret_cast<const OSENC_Feature_Identification_Record_Payload*>(b.constData());
        current = new S57::Object(p->feature_ID, p->feature_type_code, m_locations);
        objects.append(OData(current));
        return true;
      })
    },

    {SencRecordType::FEATURE_ATTRIBUTE_RECORD, new Handler([&current, helper] (const Buffer& b) {
        if (!current) return false;
        auto p = reinterpret_cast<const OSENC_Attribute_Record_Payload*>(b.constData());
        auto t = static_cast<S57::Attribute::Type>(p->attribute_value_type);
        // qDebug() << "feature attribute record" << p->attribute_type_code << S52::GetAttributeName(p->attribute_type_code) << p->attribute_value_type;
        switch (t) {
        case S57::Attribute::Type::Integer: {
          int v;
          memcpy(&v, &p->attribute_data, sizeof(int));
          helper.addAttribute(current,
                              p->attribute_type_code,
                              S57::Attribute(v));
          return true;
        }
        case S57::Attribute::Type::Real: {
          double v;
          memcpy(&v, &p->attribute_data, sizeof(double));
          helper.addAttribute(current,
                              p->attribute_type_code,
                              S57::Attribute(v));
          return true;
        }
        case S57::Attribute::Type::String: {
          const char* s = &p->attribute_data; // null terminated string
          // handles strings and integer lists
          helper.addString(current,
                           p->attribute_type_code,
                           QString::fromUtf8(s));
          return true;
        }
        case S57::Attribute::Type::None: {
          helper.addAttribute(current,
                              p->attribute_type_code,
                              S57::Attribute(S57::Attribute::Type::None));
          return true;
        }
        default: return false;
        }
        return false;
      })
    },

    {SencRecordType::FEATURE_GEOMETRY_RECORD_POINT, new Handler([&current, &objects, helper, this] (const Buffer& b) {
        // qDebug() << "feature geometry/point record";
        if (!current) return false;
        auto p = reinterpret_cast<const OSENC_PointGeometry_Record_Payload*>(b.constData());
        auto p0 = m_nativeProj->fromWGS84(WGS84Point::fromLL(p->lon, p->lat));
        objects.last().type = S57::Geometry::Type::Point;
        QRectF bb(p0 - QPointF(10, 10), QSizeF(20, 20));
        return helper.setGeometry(current, new S57::Geometry::Point(p0, m_nativeProj), bb);
      })
    },

    {SencRecordType::FEATURE_GEOMETRY_RECORD_LINE, new Handler([&objects] (const Buffer& b) {
        // qDebug() << "feature geometry/line record";
        auto p = reinterpret_cast<const OSENC_LineGeometry_Record_Payload*>(b.constData());
        LineHandle es(p->edgeVector_count * 3);
        memcpy(es.data(), &p->edge_data, p->edgeVector_count * 3 * sizeof(int));
        objects.last().lines.append(es);
        objects.last().type = S57::Geometry::Type::Line;
        return true;
      })
    },

    {SencRecordType::FEATURE_GEOMETRY_RECORD_AREA, new Handler([&objects] (const Buffer& b) {
        // qDebug() << "feature geometry/area record";
        auto p = reinterpret_cast<const OSENC_AreaGeometry_Record_Payload*>(b.constData());
        LineHandle es(p->edgeVector_count * 3);
        const uint8_t* ptr = &p->edge_data;
        // skip contour counts
        ptr += p->contour_count * sizeof(int);
        TriangleHandle ts(p->triprim_count);
        for (uint i = 0; i < p->triprim_count; i++) {
          ts[i].mode = static_cast<GLenum>(*ptr); // Note: relies on GL_TRIANGLE* to be less than 128
          ptr++;
          auto nvert = *reinterpret_cast<const uint32_t*>(ptr);
          ptr += sizeof(uint32_t);
          ptr += 4 * sizeof(double); // skip bbox
          ts[i].vertices.resize(nvert * 2);
          memcpy(ts[i].vertices.data(), ptr, nvert * 2 * sizeof(float));
          ptr += nvert * 2 * sizeof(float);
        }
        memcpy(es.data(), ptr, p->edgeVector_count * 3 * sizeof(int));
        objects.last().lines.append(es);
        objects.last().triangles.append(ts);
        objects.last().type = S57::Geometry::Type::Area;
        return true;
      })
    },

    {SencRecordType::FEATURE_GEOMETRY_RECORD_MULTIPOINT, new Handler([&current, &objects, helper, this] (const Buffer& b) {
        // qDebug() << "feature geometry/multipoint record";
        if (!current) return false;
        auto p = reinterpret_cast<const OSENC_MultipointGeometry_Record_Payload*>(b.constData());
        S57::Geometry::PointVector ps(p->point_count * 3);
        memcpy(ps.data(), &p->point_data, p->point_count * 3 * sizeof(double));
        objects.last().type = S57::Geometry::Type::Point;
        // compute bounding box
        QPointF ur(-1.e15, -1.e15);
        QPointF ll(1.e15, 1.e15);
        for (uint i = 0; i < p->point_count; i++) {
          const QPointF q(ps[3 * i], ps[3 * i + 1]);
          ur.setX(qMax(ur.x(), q.x()));
          ur.setY(qMax(ur.y(), q.y()));
          ll.setX(qMin(ll.x(), q.x()));
          ll.setY(qMin(ll.y(), q.y()));
        }
        return helper.setGeometry(current, new S57::Geometry::Point(ps, m_nativeProj), QRectF(ll, ur));
      })
    },

    {SencRecordType::VECTOR_EDGE_NODE_TABLE_RECORD, new Handler([&vertices, &edgeMap] (const Buffer& b) {
        // qDebug() << "vector edge node table record";
        const char* ptr = b.constData();

        // The Feature(Object) count
        auto cnt = *reinterpret_cast<const quint32*>(ptr);
        ptr += sizeof(quint32);

        for (uint i = 0; i < cnt; i++) {
          auto index = *reinterpret_cast<const quint32*>(ptr);
          ptr += sizeof(quint32);

          auto pcnt = *reinterpret_cast<const quint32*>(ptr);
          ptr += sizeof(quint32);

          QVector<float> edges(2 * pcnt);
          memcpy(edges.data(), ptr, 2 * pcnt * sizeof(float));
          ptr += 2 * pcnt * sizeof(float);

          EdgeExtent edge;
          edge.first = vertices.size() / 2;
          edge.last = edge.first + pcnt - 1;
          edgeMap[index] = edge;
          vertices.append(edges);
        }
        return true;
      })
    },

    {SencRecordType::VECTOR_CONNECTED_NODE_TABLE_RECORD, new Handler([&vertices, &connMap] (const Buffer& b) {
        // qDebug() << "vector connected node table record";
        const char* ptr = b.constData();

        // The Feature(Object) count
        auto cnt = *reinterpret_cast<const quint32*>(ptr);

        ptr += sizeof(quint32);

        for (uint i = 0; i < cnt; i++) {
          auto index = *reinterpret_cast<const quint32*>(ptr);
          ptr += sizeof(quint32);

          auto x = *reinterpret_cast<const float*>(ptr);
          ptr += sizeof(float);
          auto y = *reinterpret_cast<const float*>(ptr);
          ptr += sizeof(float);

          connMap[index] = vertices.size() / 2;
          vertices.append(x);
          vertices.append(y);
        }
        return true;
      })
    },

    {SencRecordType::CELL_COVR_RECORD, new Handler([this] (const Buffer& b) {
        // qDebug() << "cell coverage record";
        auto p = reinterpret_cast<const OSENC_POINT_ARRAY_Record_Payload*>(b.constData());
        Polygon poly(p->count * 2);
        memcpy(poly.data(), &p->array, p->count * 2 * sizeof(float));
        m_coverage.append(poly);
        return true;
      })
    },

    {SencRecordType::CELL_NOCOVR_RECORD, new Handler([this] (const Buffer& b) {
        qDebug() << "cell nocoverage record";
        auto p = reinterpret_cast<const OSENC_POINT_ARRAY_Record_Payload*>(b.constData());
        Polygon poly(p->count * 2);
        memcpy(poly.data(), &p->array, p->count * 2 * sizeof(float));
        m_nocoverage.append(poly);
        return true;
      })
    },
  };

  bool done = false;

  while (!done && !stream.atEnd()) {

    Buffer buffer;

    buffer.resize(sizeof(OSENC_Record_Base));
    if (stream.readRawData(buffer.data(), buffer.size()) == -1) {
      done = true;
      qWarning() << "Cannot read base record";
      continue;
    }

    auto record = reinterpret_cast<OSENC_Record_Base*>(buffer.data());

    // copy, record_type will be overwritten in the next stream.readRawData
    SencRecordType rec_type = record->record_type;

    buffer.resize(record->record_length - sizeof(OSENC_Record_Base));
    if (stream.readRawData(buffer.data(), buffer.size()) == -1) {
      done = true;
      qWarning() << "Cannot read base record data";
      continue;
    }
    if (!handlers.contains(rec_type)) {
      continue;
    }

    done = !(handlers[rec_type])->func(buffer);
    if (done) {
      qWarning() << "Handler failed for type" << as_numeric(rec_type);
    }

  }
  qDeleteAll(handlers);

  auto lineGeometry = [&indices, &vertices, edgeMap, connMap] (const LineHandle& geom, S57::ElementDataVector& elems) {
    const int cnt = geom.size() / 3;
    // qDebug() << geom;
    for (int i = 0; i < cnt;) {
      S57::ElementData e;
      e.mode = GL_LINE_STRIP_ADJACENCY;
      e.offset = indices.size() * sizeof(GLuint);
      indices.append(0); // dummy index to account adjacency
      e.count = 1;
      int np = 3 * i;
      while (i < cnt && geom[3 * i] == geom[np]) {
        const int c1 = connMap[geom[3 * i]];
        const int m = geom[3 * i + 1];
        if (edgeMap.contains(std::abs(m))) {
          const int first = edgeMap[std::abs(m)].first;
          const int last = edgeMap[std::abs(m)].last;
          e.count += addIndices(c1, first, last, m < 0, indices);
        } else {
          indices.append(c1);
          e.count += 1;
        }
        np = 3 * i + 2;
        i++;
      }
      if (i < cnt) {
        const GLuint last = connMap[geom[3 * i - 1]];
        // account adjacency
        const int adj = e.offset / sizeof(GLuint);
        const GLuint first = indices[adj + 1];
        if (last == first) {
          indices[adj] = indices.last(); // prev
          indices.append(last); // last real index
          indices.append(indices[adj + 2]); // adjacent = next of first
        } else {
          indices[adj] = addAdjacent(first, indices[adj + 2], vertices);
          indices.append(last);
          indices.append(addAdjacent(last, indices.last(), vertices));
        }
        e.count += 2;
      }
      elems.append(e);
    }
    const GLuint last = connMap[geom[geom.size() - 1]];
    // account adjacency
    const int adj = elems.last().offset / sizeof(GLuint);
    const GLuint first = indices[adj + 1];
    if (last == first) {
      indices[adj] = indices.last(); // prev
      indices.append(last); // last real index
      indices.append(indices[adj + 2]); // adjacent = next of first
    } else {
      indices.append(last); // adjacent = same as last
      indices[adj] = addAdjacent(first, indices[adj + 2], vertices);
      indices.append(last);
      indices.append(addAdjacent(last, indices.last(), vertices));
    }
    elems.last().count += 2;
  };

  auto triangleGeometry = [&vertices] (const TriangleHandle& geom, S57::ElementDataVector& elems) {
    GLint offset = 0;
    for (const TrianglePatch& p: geom) {
      S57::ElementData d;
      d.mode = p.mode;
      d.offset = offset;
      d.count = p.vertices.size() / 2;
      vertices.append(p.vertices);
      elems.append(d);
      offset += d.count;
    }
  };

  for (OData& d: objects) {

    // setup meta, line and area geometries
    if (d.type == S57::Geometry::Type::Meta) {
      helper.setGeometry(d.object, new S57::Geometry::Meta(), QRectF());
    } else if (d.type == S57::Geometry::Type::Line) {

      S57::ElementDataVector elems;
      lineGeometry(d.lines, elems);

      const QPointF c = computeLineCenter(elems, vertices, indices);
      const QRectF bbox = computeBBox(elems, vertices, indices);
      helper.setGeometry(d.object,
                         new S57::Geometry::Line(elems, c, 0, m_nativeProj),
                         bbox);

    } else if (d.type == S57::Geometry::Type::Area) {

      S57::ElementDataVector lelems;
      lineGeometry(d.lines, lelems);

      GLsizei offset = vertices.size() * sizeof(GLfloat);
      S57::ElementDataVector telems;
      triangleGeometry(d.triangles, telems);

      const QPointF c = computeAreaCenter(telems, vertices, offset);
      const QRectF bbox = computeBBox(lelems, vertices, indices);
      helper.setGeometry(d.object,
                         new S57::Geometry::Area(lelems, c, telems, offset, false, m_nativeProj),
                         bbox);
    }

    // bind objects to lookup records
    S52::Lookup* lp = S52::FindLookup(d.object);
    m_lookups.append(ObjectLookup(d.object, lp));

    // sort point objects by their locations
    const WGS84Point p = d.object->geometry()->centerLL();
    if (p.valid()) {
      m_locations.insert(p, d.object);
    }
  }

  // fill in the buffers

  if (!m_coordBuffer.create()) qFatal("No can do");
  m_coordBuffer.setUsagePattern(QOpenGLBuffer::DynamicDraw);
  m_coordBuffer.bind();
  m_staticVertexOffset = vertices.size() * sizeof(GLfloat);
  // add 10% extra space for vertices added later
  m_coordBuffer.allocate(m_staticVertexOffset + m_staticVertexOffset / 10);
  m_coordBuffer.write(0, vertices.constData(), m_staticVertexOffset);

  m_indexBuffer.create();
  m_indexBuffer.setUsagePattern(QOpenGLBuffer::DynamicDraw);
  m_indexBuffer.bind();
  m_staticElemOffset = indices.size() * sizeof(GLuint);
  // add 10% extra space for elements added later
  m_indexBuffer.allocate(m_staticElemOffset + m_staticElemOffset / 10);
  m_indexBuffer.write(0, indices.constData(), m_staticElemOffset);

  m_pivotBuffer.create();
  m_pivotBuffer.setUsagePattern(QOpenGLBuffer::DynamicDraw);
  m_pivotBuffer.bind();
  // 5K raster symbol/pattern instances
  m_pivotBuffer.allocate(5000 * 2 * sizeof(GLfloat));

}

S57Chart::~S57Chart() {
  for (ObjectLookup& d: m_lookups) {
    delete d.object;
  }
  for (S57::PaintDataMap& d: m_paintData) {
    for (S57::PaintMutIterator it = d.begin(); it != d.end(); ++it) {
      delete it.value();
    }
  }
  delete m_nativeProj;
}




void S57Chart::finalizePaintData() {
  // clear old paint data
  for (S57::PaintDataMap& d: m_paintData) {
    for (S57::PaintMutIterator it = d.begin(); it != d.end(); ++it) {
      delete it.value();
    }
  }

  m_paintData = m_updatedPaintData;

  // update vertex buffer
  m_coordBuffer.bind();
  GLsizei dataLen = m_staticVertexOffset + sizeof(GLfloat) * m_updatedVertices.size();
  if (dataLen > m_coordBuffer.size()) {
    auto staticData = new uchar[m_staticVertexOffset];
    auto coords = m_coordBuffer.map(QOpenGLBuffer::ReadOnly);
    memcpy(staticData, coords, m_staticVertexOffset);
    m_coordBuffer.unmap();
    // add 10% extra
    m_coordBuffer.allocate(dataLen + dataLen / 10);
    m_coordBuffer.write(0, staticData, m_staticVertexOffset);
    delete [] staticData;
  }

  m_coordBuffer.write(m_staticVertexOffset, m_updatedVertices.constData(), dataLen - m_staticVertexOffset);

  // update pivot buffer
  m_pivotBuffer.bind();
  dataLen = sizeof(GLfloat) * m_updatedPivots.size();
  if (dataLen > m_pivotBuffer.size()) {
    m_pivotBuffer.allocate(dataLen);
  }

  m_pivotBuffer.write(0, m_updatedPivots.constData(), dataLen);

}

void S57Chart::updatePaintData(const QRectF &viewArea, quint32 scale) {

  // clear old updates
  for (S57::PaintDataMap& d: m_updatedPaintData) d.clear();
  m_updatedVertices.clear();
  m_updatedPivots.clear();

  auto maxcat = as_numeric(m_settings->maxCategory());
  auto today = QDate::currentDate();

  const qreal sf = scaleFactor(viewArea, scale);

  SymbolPriorityVector symbols(S52::Lookup::PriorityCount);

  using Handler = std::function<void (const S57::PaintMutIterator&, int)>;

  auto parseLocals = [] (S57::PaintData::Type t, S57::PaintDataMap& pd, int prio, Handler func) {
    S57::PaintMutIterator it = pd.find(t);
    while (it != pd.end() && it.key() == t) {
      func(it, prio);
      it = pd.erase(it);
    }
  };

  auto handleLine = [this] (const S57::PaintMutIterator& it, int prio) {
    auto p = dynamic_cast<S57::Globalizer*>(it.value());
    auto pn = p->globalize(m_staticVertexOffset + m_updatedVertices.size());
    m_updatedVertices += p->vertices();
    delete p;
    m_updatedPaintData[prio].insert(pn->type(), pn);
  };

  auto mergeSymbols = [this, sf, &symbols] (S57::PaintMutIterator it, int prio) {
    auto s = dynamic_cast<S57::SymbolMerger*>(it.value());
    if (symbols[prio].contains(s->key())) {
      auto s0 = dynamic_cast<S57::SymbolMerger*>(symbols[prio][s->key()]);
      s0->merge(s, sf);
    } else {
      s->merge(nullptr, sf);
      symbols[prio][s->key()] = it.value();
    }
  };

  for (ObjectLookup& d: m_lookups) {
    // check bbox & scale
    if (!d.object->canPaint(viewArea, scale, today)) continue;

    S57::PaintDataMap pd = d.lookup->execute(d.object);

    // check category
    if (pd.contains(S57::PaintData::Type::CategoryOverride)) {
      pd.remove(S57::PaintData::Type::CategoryOverride);
    } else if (as_numeric(d.lookup->category()) > maxcat) {
      continue;
    }

    const int prio = d.lookup->priority();
    // handle the local arrays
    parseLocals(S57::PaintData::Type::SolidLineLocal, pd, prio, handleLine);
    parseLocals(S57::PaintData::Type::DashedLineLocal, pd, prio, handleLine);

    // merge symbols & patterns
    parseLocals(S57::PaintData::Type::RasterSymbolElements, pd, prio, mergeSymbols);
    parseLocals(S57::PaintData::Type::RasterPatterns, pd, prio, mergeSymbols);

    m_updatedPaintData[prio] += pd;

  }

  // move merged symbols & patterns back to paintdatamap
  for (int i = 0; i < S52::Lookup::PriorityCount; i++) {
    SymbolMap merged = symbols[i];
    for (SymbolMutIterator it = merged.begin(); it != merged.end(); ++it) {
      auto r = dynamic_cast<S57::RasterData*>(it.value());
      r->getPivots(m_updatedPivots);
      m_updatedPaintData[i].insert(it.value()->type(), it.value());
    }
  }
}

qreal S57Chart::scaleFactor(const QRectF &va, quint32 scale) {
  const WGS84Point p1 = m_nativeProj->toWGS84(va.center());
  const WGS84Point p2 = m_nativeProj->toWGS84(QPoint(.5 * (va.left() + va.right()), va.bottom()));

  // ratio of screen pixel height and viewarea height
  return 2000. * (p2 - p1).meters() / scale * dots_per_mm_y / va.height();
}



static QRectF computeBBox(const S57::ElementDataVector &elems,
                          const GL::VertexVector& vertices,
                          const GL::IndexVector& indices) {

  QPointF ur(-1.e15, -1.e15);
  QPointF ll(1.e15, 1.e15);

  for (const S57::ElementData& elem: elems) {
    const int first = elem.offset / sizeof(GLuint);
    for (uint i = 0; i < elem.count; i++) {
      const int index = indices[first + i];
      QPointF q(vertices[2 * index], vertices[2 * index + 1]);
      ur.setX(qMax(ur.x(), q.x()));
      ur.setY(qMax(ur.y(), q.y()));
      ll.setX(qMin(ll.x(), q.x()));
      ll.setY(qMin(ll.y(), q.y()));
    }
  }
  return QRectF(ll, ur); // inverted y-axis
}

static QPointF computeLineCenter(const S57::ElementDataVector &elems,
                                 const GL::VertexVector& vertices,
                                 const GL::IndexVector& indices) {
  int first = elems[0].offset / sizeof(GLuint) + 1; // account adjacency
  int last = first + elems[0].count - 3; // account adjacency
  if (elems.size() > 1 || indices[first] == indices[last]) {
    // several lines or closed loops: compute center of gravity
    QPointF s(0, 0);
    int n = 0;
    for (auto& elem: elems) {
      first = elem.offset / sizeof(GLuint) + 1; // account adjacency
      last = first + elem.count - 3; // account adjacency
      for (int i = first; i <= last; i++) {
        const int index = indices[i];
        s.rx() += vertices[2 * index + 0];
        s.ry() += vertices[2 * index + 1];
      }
      n += elem.count - 2;
    }
    return s / n;
  }
  // single open line: compute mid point of running length
  QVector<float> lengths;
  float len = 0;
  for (int i = first; i < last; i++) {
    const int i1 = indices[i + 1];
    const int i0 = indices[i];
    const QPointF d(vertices[2 * i1] - vertices[2 * i0], vertices[2 * i1 + 1] - vertices[2 * i0 + 1]);
    lengths.append(sqrt(QPointF::dotProduct(d, d)));
    len += lengths.last();
  }
  const float halfLen = len / 2;
  len = 0;
  int i = 0;
  while (i < lengths.size() && len < halfLen) {
    len += lengths[i];
    i++;
  }
  const int i1 = indices[first + i];
  const int i0 = indices[first + i - 1];
  const QPointF p1(vertices[2 * i1], vertices[2 * i1 + 1]);
  const QPointF p0(vertices[2 * i0], vertices[2 * i0 + 1]);
  return p0 + (len - halfLen) * (p1 - p0);

}

static QPointF computeAreaCenter(const S57::ElementDataVector &elems,
                                 const GL::VertexVector& vertices,
                                 GLsizei offset) {
  float area = 0;
  QPoint s(0, 0);
  for (const S57::ElementData& elem: elems) {
    if (elem.mode == GL_TRIANGLES) {
      int first = offset / sizeof(GLfloat) + 2 * elem.offset;
      for (uint i = 0; i < elem.count / 3; i++) {
        const QPointF p0(vertices[first + 6 * i + 0], vertices[first + 6 * i + 1]);
        const QPointF p1(vertices[first + 6 * i + 2], vertices[first + 6 * i + 3]);
        const QPointF p2(vertices[first + 6 * i + 4], vertices[first + 6 * i + 5]);
        const float da = std::abs((p1.x() - p0.x()) * (p2.y() - p0.y()) - (p2.x() - p0.x()) * (p1.y() - p0.y()));
        area += da;
        s.rx() += da / 3. * (p0.x() + p1.x() + p2.x());
        s.ry() += da / 3. * (p0.y() + p1.y() + p2.y());
      }
    } else if (elem.mode == GL_TRIANGLE_STRIP) {
      int first = offset / sizeof(GLfloat) + 2 * elem.offset;
      for (uint i = 0; i < elem.count - 2; i++) {
        const QPointF p0(vertices[first + 2 * i + 0], vertices[first + 2 * i + 1]);
        const QPointF p1(vertices[first + 2 * i + 2], vertices[first + 2 * i + 3]);
        const QPointF p2(vertices[first + 2 * i + 4], vertices[first + 2 * i + 5]);
        const float da = std::abs((p1.x() - p0.x()) * (p2.y() - p0.y()) - (p2.x() - p0.x()) * (p1.y() - p0.y()));
        area += da;
        s.rx() += da / 3. * (p0.x() + p1.x() + p2.x());
        s.ry() += da / 3. * (p0.y() + p1.y() + p2.y());
      }
    } else if (elem.mode == GL_TRIANGLE_FAN) {
      int first = offset / sizeof(GLfloat) + 2 * elem.offset;
      const QPointF p0(vertices[first + 0], vertices[first + 1]);
      for (uint i = 0; i < elem.count - 2; i++) {
        const QPointF p1(vertices[first + 2 * i + 2], vertices[first + 2 * i + 3]);
        const QPointF p2(vertices[first + 2 * i + 4], vertices[first + 2 * i + 5]);
        const float da = std::abs((p1.x() - p0.x()) * (p2.y() - p0.y()) - (p2.x() - p0.x()) * (p1.y() - p0.y()));
        area += da;
        s.rx() += da / 3. * (p0.x() + p1.x() + p2.x());
        s.ry() += da / 3. * (p0.y() + p1.y() + p2.y());
      }
    } else {
      Q_ASSERT_X(false, "computeAreaCenter", "Unknown primitive");
    }
  }

  return s / area;
}


void S57Chart::drawAreas(const Camera* cam, int prio) {

  m_coordBuffer.bind();
  m_indexBuffer.bind();

  auto prog = GL::AreaShader::instance();

  const QPointF p = cam->geoprojection()->fromWGS84(geoProjection()->reference());
  prog->setGlobals(cam, p);
  prog->setDepth(prio);


  auto f = QOpenGLContext::currentContext()->extraFunctions();

  const S57::PaintIterator end = m_paintData[prio].constEnd();

  S57::PaintIterator arr = m_paintData[prio].constFind(S57::PaintData::Type::TriangleArrays);

  while (arr != end && arr.key() == S57::PaintData::Type::TriangleArrays) {
    auto d = dynamic_cast<const S57::TriangleArrayData*>(arr.value());
    d->setUniforms();
    d->setVertexOffset();
    for (const S57::ElementData& e: d->elements()) {
      f->glDrawArrays(e.mode, e.offset, e.count);
    }
    ++arr;
  }

  S57::PaintIterator elem = m_paintData[prio].constFind(S57::PaintData::Type::TriangleElements);

  while (elem != end && elem.key() == S57::PaintData::Type::TriangleElements) {
    auto d = dynamic_cast<const S57::TriangleElemData*>(elem.value());
    d->setUniforms();
    d->setVertexOffset();
    for (const S57::ElementData& e: d->elements()) {
      f->glDrawElements(e.mode, e.count, GL_UNSIGNED_INT,
                        reinterpret_cast<const void*>(e.offset));
    }
    ++elem;
  }

  // stencil pattern areas

  f->glEnable(GL_STENCIL_TEST);
  f->glDisable(GL_DEPTH_TEST);
  f->glStencilMask(0xff);
  f->glStencilFunc(GL_ALWAYS, 1, 0xff);
  f->glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
  f->glColorMask(false, false, false, false);

  arr = m_paintData[prio].constFind(S57::PaintData::Type::RasterPatterns);

  using Data = S57::RasterPatternData::AreaData;
  while (arr != end && arr.key() == S57::PaintData::Type::RasterPatterns) {
    auto d = dynamic_cast<const S57::RasterPatternData*>(arr.value());
    for (const Data& rd: d->areaArrays()) {
      d->setAreaVertexOffset(rd.vertexOffset);
      for (const S57::ElementData& e: rd.elements) {
        f->glDrawArrays(e.mode, e.offset, e.count);
      }
    }
    for (const Data& rd: d->areaElements()) {
      d->setAreaVertexOffset(rd.vertexOffset);
      for (const S57::ElementData& e: rd.elements) {
        f->glDrawElements(e.mode, e.count, GL_UNSIGNED_INT,
                          reinterpret_cast<const void*>(e.offset));
      }
    }
    ++arr;
  }

  f->glDisable(GL_STENCIL_TEST);
  f->glEnable(GL_DEPTH_TEST);
  f->glStencilMask(0x00);
  f->glColorMask(true, true, true, true);
}

void S57Chart::drawSolidLines(const Camera* cam, int prio) {

  m_coordBuffer.bind();
  m_indexBuffer.bind();

  auto prog = GL::SolidLineShader::instance();

  const QPointF p = cam->geoprojection()->fromWGS84(geoProjection()->reference());
  prog->setGlobals(cam, p);
  prog->setDepth(prio);


  auto f = QOpenGLContext::currentContext()->extraFunctions();

  const S57::PaintIterator end = m_paintData[prio].constEnd();

  S57::PaintIterator arr = m_paintData[prio].constFind(S57::PaintData::Type::SolidLineArrays);

  while (arr != end && arr.key() == S57::PaintData::Type::SolidLineArrays) {
    auto d = dynamic_cast<const S57::SolidLineArrayData*>(arr.value());
    d->setUniforms();
    d->setVertexOffset();
    for (const S57::ElementData& e: d->elements()) {
      f->glDrawArrays(e.mode, e.offset, e.count);
    }
    ++arr;
  }

  S57::PaintIterator elem = m_paintData[prio].constFind(S57::PaintData::Type::SolidLineElements);

  while (elem != end && elem.key() == S57::PaintData::Type::SolidLineElements) {
    auto d = dynamic_cast<const S57::SolidLineElemData*>(elem.value());
    d->setUniforms();
    d->setVertexOffset();
    for (const S57::ElementData& e: d->elements()) {
      f->glDrawElements(e.mode, e.count, GL_UNSIGNED_INT,
                        reinterpret_cast<const void*>(e.offset));
    }
    ++elem;
  }
}

void S57Chart::drawDashedLines(const Camera* cam, int prio) {

  m_coordBuffer.bind();
  m_indexBuffer.bind();

  auto prog = GL::DashedLineShader::instance();
  const QPointF p = cam->geoprojection()->fromWGS84(geoProjection()->reference());
  prog->setGlobals(cam, p);
  prog->setDepth(prio);

  auto f = QOpenGLContext::currentContext()->extraFunctions();


  const S57::PaintIterator end = m_paintData[prio].constEnd();

  S57::PaintIterator arr = m_paintData[prio].constFind(S57::PaintData::Type::DashedLineArrays);

  while (arr != end && arr.key() == S57::PaintData::Type::DashedLineArrays) {
    auto d = dynamic_cast<const S57::DashedLineArrayData*>(arr.value());
    d->setUniforms();
    d->setVertexOffset();
    for (const S57::ElementData& e: d->elements()) {
      f->glDrawArrays(e.mode, e.offset, e.count);
    }
    ++arr;
  }

  S57::PaintIterator elem = m_paintData[prio].constFind(S57::PaintData::Type::DashedLineElements);

  while (elem != end && elem.key() == S57::PaintData::Type::DashedLineElements) {
    auto d = dynamic_cast<const S57::DashedLineElemData*>(elem.value());
    d->setUniforms();
    d->setVertexOffset();
    for (const S57::ElementData& e: d->elements()) {
      f->glDrawElements(e.mode, e.count, GL_UNSIGNED_INT,
                        reinterpret_cast<const void*>(e.offset));
    }
    ++elem;
  }
}


void S57Chart::drawText(const Camera* cam, int prio) {

  TextManager::instance()->bind();

  auto prog = GL::TextShader::instance();

  const QPointF p = cam->geoprojection()->fromWGS84(geoProjection()->reference());
  prog->setGlobals(cam, p);
  prog->setDepth(prio);

  auto f = QOpenGLContext::currentContext()->extraFunctions();

  const S57::PaintIterator end = m_paintData[prio].constEnd();
  S57::PaintIterator elem = m_paintData[prio].constFind(S57::PaintData::Type::TextElements);

  while (elem != end && elem.key() == S57::PaintData::Type::TextElements) {
    auto d = dynamic_cast<const S57::TextElemData*>(elem.value());
    d->setUniforms();
    d->setVertexOffset();
    f->glDrawElements(d->elements().mode, d->elements().count, GL_UNSIGNED_INT,
                      reinterpret_cast<const void*>(d->elements().offset));
    ++elem;
  }
}

void S57Chart::drawRasterSymbols(const Camera* cam, int prio) {


  RasterSymbolManager::instance()->bind();

  auto prog = GL::RasterSymbolShader::instance();

  const QPointF p = cam->geoprojection()->fromWGS84(geoProjection()->reference());
  prog->setGlobals(cam, p);
  prog->setDepth(prio);

  auto f = QOpenGLContext::currentContext()->extraFunctions();

  const S57::PaintIterator end = m_paintData[prio].constEnd();
  S57::PaintIterator elem = m_paintData[prio].constFind(S57::PaintData::Type::RasterSymbolElements);

  while (elem != end && elem.key() == S57::PaintData::Type::RasterSymbolElements) {
    auto d = dynamic_cast<const S57::RasterSymbolData*>(elem.value());
    d->setUniforms();
    m_pivotBuffer.bind();
    d->setVertexOffset();
    auto e = d->elements();
    f->glDrawElementsInstanced(e.mode,
                               e.count,
                               GL_UNSIGNED_INT,
                               reinterpret_cast<const void*>(e.offset),
                               d->count());
    ++elem;
  }
}


void S57Chart::drawPatterns(const Camera *cam) {

  RasterSymbolManager::instance()->bind();

  auto prog = GL::RasterSymbolShader::instance();

  const QPointF p = cam->geoprojection()->fromWGS84(geoProjection()->reference());
  prog->setGlobals(cam, p);

  auto f = QOpenGLContext::currentContext()->extraFunctions();
  f->glEnable(GL_STENCIL_TEST);
  f->glStencilMask(0xff);
  f->glStencilFunc(GL_EQUAL, 1, 0xff);
  f->glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

  for (int prio = 0; prio < S52::Lookup::PriorityCount; prio++) {
    S57::PaintIterator end = m_paintData[prio].constEnd();

    prog->setDepth(prio);

    S57::PaintIterator arr = m_paintData[prio].constFind(S57::PaintData::Type::RasterPatterns);
    while (arr != end && arr.key() == S57::PaintData::Type::RasterPatterns) {
      auto d = dynamic_cast<const S57::RasterPatternData*>(arr.value());
      d->setUniforms();
      m_pivotBuffer.bind();
      d->setVertexOffset();

      auto e = d->elements();
      f->glDrawElementsInstanced(e.mode,
                                 e.count,
                                 GL_UNSIGNED_INT,
                                 reinterpret_cast<const void*>(e.offset),
                                 d->count());
      ++arr;
    }
  }

  f->glDisable(GL_STENCIL_TEST);
  f->glClear(GL_STENCIL_BUFFER_BIT);
  f->glStencilMask(0x00);
}

static GLsizei addIndices(GLuint first, GLuint mid1, GLuint mid2, bool reversed, GL::IndexVector& indices) {
  indices.append(first);
  if (!reversed) {
    for (uint i = mid1; i <= mid2; i++) {
      indices.append(i);
    }
  } else {
    for (int i = mid2; i >= static_cast<int>(mid1); i--) {
      indices.append(i);
    }
  }
  return 1 + mid2 - mid1 + 1;
}

static GLuint addAdjacent(GLuint base, GLuint next, GL::VertexVector& vertices) {
  const float x1 = vertices[2 * base];
  const float y1 = vertices[2 * base + 1];
  const float x2 = vertices[2 * next];
  const float y2 = vertices[2 * next + 1];
  vertices.append(2 * x1 - x2);
  vertices.append(2 * y1 - y2);
  return (vertices.size() - 1) / 2;
}

