#include "s57chart.h"
#include <QFile>
#include <QDataStream>
#include <QVector>
#include <functional>
#include "geoprojection.h"
#include "s57object.h"
#include "s52presentation.h"
#include "drawable.h"
#include "earcut.hpp"
#include "triangleoptimizer.h"
#include <QDate>
#include "settings.h"

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
  auto p16 = reinterpret_cast<quint16*>(buffer.data());
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
        const quint16* p16 = reinterpret_cast<const quint16*>(b.constData());
        // qDebug() << "cell edition" << *p16;
        return true;
      })
    },
    {SencRecordType::HEADER_CELL_UPDATE, new Handler([] (const Buffer& b) {
        const quint16* p16 = reinterpret_cast<const quint16*>(b.constData());
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

S57Chart::S57Chart(quint32 id, const QString& path, const GeoProjection* proj)
  : QObject(nullptr)
  , m_nativeProj(new SimpleMercator)
  , m_paintData(S52::Lookup::PriorityCount)
  , m_id(id)
  , m_settings(Settings::instance())
{
  S57ChartOutline outline(path);
  m_extent = outline.extent();
  m_nativeProj->setReference(outline.reference());

  if (*proj != *m_nativeProj) {
    throw NotImplementedError("Only SimpleMercator supported for now");
  }

  QFile file(path);
  file.open(QFile::ReadOnly);
  QDataStream stream(&file);

  S57::Object* current = nullptr;
  S57::ObjectBuilder helper;

  using GeomHandle = QVector<int>;
  struct OData {
    OData(S57::Object* obj): object(obj), geometry(), type(S57::Geometry::Type::Meta) {}
    S57::Object* object;
    GeomHandle geometry;
    S57::Geometry::Type type;
  };

  QVector<OData> objects;

  struct EdgeExtent {
    int first;
    int last;
  };

  QMap<int, int> connMap;
  QMap<int, EdgeExtent> edgeMap;


  const QMap<SencRecordType, Handler*> handlers {

    {SencRecordType::FEATURE_ID_RECORD, new Handler([&current, &objects] (const Buffer& b) {
        // qDebug() << "feature id record";
        auto p = reinterpret_cast<const OSENC_Feature_Identification_Record_Payload*>(b.constData());
        current = new S57::Object(p->feature_ID, p->feature_type_code);
        objects.append(OData(current));
        return true;
      })
    },

    {SencRecordType::FEATURE_ATTRIBUTE_RECORD, new Handler([&current, helper] (const Buffer& b) {
        // qDebug() << "feature attribute record";
        if (!current) return false;
        auto p = reinterpret_cast<const OSENC_Attribute_Record_Payload*>(b.constData());
        auto t = static_cast<S57::Attribute::Type>(p->attribute_value_type);
        switch (t) {
        case S57::Attribute::Type::Integer: {
          int v;
          memcpy(&v, &p->attribute_data, sizeof(int));
          helper.addAttribute(current,
                              p->attribute_type_code,
                              S57::Attribute(v));
          return true;
        }
        case S57::Attribute::Type::IntegerList: {
          int bytes = b.size() - sizeof(OSENC_Attribute_Record_Payload) + 1;
          QVector<int> vs(bytes / sizeof(int));
          memcpy(vs.data(), &p->attribute_data, bytes);
          helper.addAttribute(current,
                              p->attribute_type_code,
                              S57::Attribute(vs));
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
          helper.addAttribute(current,
                              p->attribute_type_code,
                              S57::Attribute(QString::fromUtf8(s)));
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
        return helper.setGeometry(current, new S57::Geometry::Point(p0), bb);
      })
    },

    {SencRecordType::FEATURE_GEOMETRY_RECORD_LINE, new Handler([&objects] (const Buffer& b) {
        // qDebug() << "feature geometry/line record";
        auto p = reinterpret_cast<const OSENC_LineGeometry_Record_Payload*>(b.constData());
        GeomHandle es(p->edgeVector_count * 3);
        memcpy(es.data(), &p->edge_data, p->edgeVector_count * 3 * sizeof(int));
        objects.last().geometry.append(es);
        objects.last().type = S57::Geometry::Type::Line;
        return true;
      })
    },

    {SencRecordType::FEATURE_GEOMETRY_RECORD_AREA, new Handler([&objects] (const Buffer& b) {
        // qDebug() << "feature geometry/area record";
        auto p = reinterpret_cast<const OSENC_AreaGeometry_Record_Payload*>(b.constData());
        GeomHandle es(p->edgeVector_count * 3);
        const uint8_t* ptr = &p->edge_data;
        // skip contour counts
        ptr += p->contour_count * sizeof(int);
        for (int i = 0; i < p->triprim_count; i++){
          ptr++; // skip triangle mode
          auto nvert = *(uint32_t *) ptr;
          ptr += sizeof(uint32_t);
          ptr += 4 * sizeof(double); // skip bbox
          ptr += nvert * 2 * sizeof(float); // skip vertices
        }
        memcpy(es.data(), ptr, p->edgeVector_count * 3 * sizeof(int));
        objects.last().geometry.append(es);
        objects.last().type = S57::Geometry::Type::Area;
        return true;
      })
    },

    {SencRecordType::FEATURE_GEOMETRY_RECORD_MULTIPOINT, new Handler([&current, &objects, helper] (const Buffer& b) {
        // qDebug() << "feature geometry/multipoint record";
        if (!current) return false;
        auto p = reinterpret_cast<const OSENC_MultipointGeometry_Record_Payload*>(b.constData());
        S57::Geometry::PointVector ps(p->point_count * 3);
        memcpy(ps.data(), &p->point_data, p->point_count * 3 * sizeof(double));
        objects.last().type = S57::Geometry::Type::Point;
        // compute bounding box
        QPointF ur(-1.e15, -1.e15);
        QPointF ll(1.e15, 1.e15);
        for (int i = 0; i < p->point_count; i++) {
          const QPointF q(ps[3 * i], ps[3 * i + 1]);
          ur.setX(qMax(ur.x(), q.x()));
          ur.setY(qMax(ur.y(), q.y()));
          ll.setX(qMin(ll.x(), q.x()));
          ll.setY(qMin(ll.y(), q.y()));
        }
        return helper.setGeometry(current, new S57::Geometry::Point(ps), QRectF(ll, ur));
      })
    },

    {SencRecordType::VECTOR_EDGE_NODE_TABLE_RECORD, new Handler([this, &edgeMap] (const Buffer& b) {
        // qDebug() << "vector edge node table record";
        const char* ptr = b.constData();

        // The Feature(Object) count
        auto cnt = *(const quint32 *) ptr;

        ptr += sizeof(quint32);

        for (int i = 0; i < cnt; i++) {
          int index = *(const quint32*) ptr;
          ptr += sizeof(quint32);

          auto pcnt = *(const quint32*) ptr;
          ptr += sizeof(quint32);

          QVector<float> edges(2 * pcnt);
          memcpy(edges.data(), ptr, 2 * pcnt * sizeof(float));
          ptr += 2 * pcnt * sizeof(float);

          EdgeExtent edge;
          edge.first = m_vertices.size() / 2;
          edge.last = edge.first + pcnt - 1;
          edgeMap[index] = edge;
          m_vertices.append(edges);
        }
        return true;
      })
    },

    {SencRecordType::VECTOR_CONNECTED_NODE_TABLE_RECORD, new Handler([this, &connMap] (const Buffer& b) {
        // qDebug() << "vector connected node table record";
        const char* ptr = b.constData();

        // The Feature(Object) count
        auto cnt = *(const quint32 *) ptr;

        ptr += sizeof(quint32);

        for (int i = 0; i < cnt; i++) {
          int index = *(const quint32*) ptr;
          ptr += sizeof(quint32);

          const float x = *(const float*) ptr;
          ptr += sizeof(float);
          const float y = *(const float*) ptr;
          ptr += sizeof(float);

          connMap[index] = m_vertices.size() / 2;
          m_vertices.append(x);
          m_vertices.append(y);
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

  auto lineGeometry = [this, edgeMap, connMap] (const GeomHandle& geom, S57::ElementDataVector& elems) {
    const int cnt = geom.size() / 3;
    // qDebug() << geom;
    for (int i = 0; i < cnt;) {
      S57::ElementData e;
      e.elementOffset = m_indices.size() * sizeof(GLuint);
      m_indices.append(0); // dummy index to account adjacency
      e.elementCount = 1;
      int np = 3 * i;
      while (i < cnt && geom[3 * i] == geom[np]) {
        const int c1 = connMap[geom[3 * i]];
        const int m = geom[3 * i + 1];
        if (edgeMap.contains(std::abs(m))) {
          const int first = edgeMap[std::abs(m)].first;
          const int last = edgeMap[std::abs(m)].last;
          e.elementCount += addIndices(c1, first, last, m < 0);
        } else {
          m_indices.append(c1);
          e.elementCount += 1;
        }
        np = 3 * i + 2;
        i++;
      }
      if (i < cnt) {
        const GLuint last = connMap[geom[3 * i - 1]];
        // account adjacency
        const int adj = e.elementOffset / sizeof(GLuint);
        const GLuint first = m_indices[adj + 1];
        if (last == first) {
          m_indices[adj] = m_indices.last(); // prev
          m_indices.append(last); // last real index
          m_indices.append(m_indices[adj + 2]); // adjacent = next of first
        } else {
          m_indices[adj] = first; // adjacent = same as first
          m_indices.append(last);
          m_indices.append(last); // adjacent = same as last
        }
        e.elementCount += 2;
      }
      elems.append(e);
    }
    const GLuint last = connMap[geom[geom.size() - 1]];
    // account adjacency
    const int adj = elems.last().elementOffset / sizeof(GLuint);
    const GLuint first = m_indices[adj + 1];
    if (last == first) {
      m_indices[adj] = m_indices.last(); // prev
      m_indices.append(last); // last real index
      m_indices.append(m_indices[adj + 2]); // adjacent = next of first
    } else {
      m_indices[adj] = first; // adjacent = same as first
      m_indices.append(last);
      m_indices.append(last); // adjacent = same as last
    }
    elems.last().elementCount += 2;
  };

  for (OData& d: objects) {

    // setup meta, line and area geometries
    if (d.type == S57::Geometry::Type::Meta) {
      helper.setGeometry(d.object, new S57::Geometry::Meta(), QRectF());
    } else if (d.type == S57::Geometry::Type::Line) {

      S57::ElementDataVector elems;
      lineGeometry(d.geometry, elems);
      helper.setGeometry(d.object,
                         new S57::Geometry::Line(elems, 0),
                         computeBBox(elems));

    } else if (d.type == S57::Geometry::Type::Area) {

      S57::ElementDataVector lelems;
      lineGeometry(d.geometry, lelems);

      // triangulate & add triangle indices
      S57::ElementDataVector telems;
      triangulate(lelems, telems);

      helper.setGeometry(d.object,
                         new S57::Geometry::Area(lelems, telems, 0),
                         computeBBox(lelems));
    }

    // bind objects to lookup records
    S52::Lookup* lp = S52::FindLookup(d.object);
    m_lookups.append(ObjectLookup(d.object, lp));
  }
}


void S57Chart::triangulate(const S57::ElementDataVector& lelems, S57::ElementDataVector& telems) {

  // The number type to use for tessellation
  using Coord = GLfloat;

  // The index type. Defaults to uint32_t, but you can also pass uint16_t if you know that your
  // data won't have more than 65536 vertices.
  using N = GLuint;

  // Create array
  using Point = std::array<Coord, 2>;



  // Fill polygon structure with actual data. Any winding order works.
  // The first polyline defines the main polygon.

  // adjacency: extra initial index, two extras at the end
  int last = m_indices.size() - 3;
  int first;
  for (int k = lelems.size() - 1; k >= 0; k--) {
    first = last - lelems[k].elementCount + 4;
    std::vector<Point> ring;
    for (int i = first; i <= last; i++) {
      const int index = m_indices[i];
      const Point p{m_vertices[2 * index], m_vertices[2 * index + 1]};
      ring.push_back(p);
    }
    std::vector<std::vector<Point>> polygon;
    polygon.push_back(ring);
    // qDebug() << "number of vertices" << ring.size();

    // Run tessellation
    // Returns array of indices that refer to the vertices of the input polygon.
    // Three subsequent indices form a triangle. Output triangles are clockwise.
    std::vector<N> indices = mapbox::earcut<N>(polygon);

    QVector<N> triangles;
    const GLsizei triCount = indices.size() / 3;
    // add triangle indices in ccw order
    for (int i = 0; i < triCount; i++)  {
      const N i0 = m_indices[first + indices[3 * i]];
      const N i1 = m_indices[first + indices[3 * i + 1]];
      const N i2 = m_indices[first + indices[3 * i + 2]];
      triangles.append(i0);
      triangles.append(i2);
      triangles.append(i1);
    }
    // qDebug() << "number of triangles" << triangles.size() / 3;

    AC::TriangleOptimizer stripper(triangles);

    for (const QVector<N>& strip: stripper.strips()) {
      S57::ElementData d;
      d.elementCount = strip.size();
      d.elementOffset = m_indices.size() * sizeof(GLuint);
      m_indices.append(strip);
      telems.append(d);
    }

    last -= lelems[k].elementCount;
  }

}


GLsizei S57Chart::addIndices(GLuint first, GLuint mid1, GLuint mid2, bool reversed) {
  m_indices.append(first);
  if (!reversed) {
    for (int i = mid1; i <= mid2; i++) {
      m_indices.append(i);
    }
  } else {
    for (int i = mid2; i >= (int) mid1; i--) {
      m_indices.append(i);
    }
  }
  return 1 + mid2 - mid1 + 1;
}

void S57Chart::updatePaintData(const QRectF &viewArea, quint32 scale) {
  for (auto& d: m_paintData) d.clear();

  auto maxcat = as_numeric(m_settings->maxCategory());
  auto today = QDate::currentDate();

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

    for (auto it = pd.constBegin(); it != pd.constEnd(); ++it) {
      m_paintData[d.lookup->priority()][it.key()].append(it.value());
    }
  }
}


QRectF S57Chart::computeBBox(const S57::ElementDataVector &elems) {
  QPointF ur(-1.e15, -1.e15);
  QPointF ll(1.e15, 1.e15);

  for (const S57::ElementData& elem: elems) {
    const int first = elem.elementOffset / sizeof(GLuint);
    for (int i = 0; i < elem.elementCount; i++) {
      const int index = m_indices[first + i];
      QPointF q(m_vertices[2 * index], m_vertices[2 * index + 1]);
      ur.setX(qMax(ur.x(), q.x()));
      ur.setY(qMax(ur.y(), q.y()));
      ll.setX(qMin(ll.x(), q.x()));
      ll.setY(qMin(ll.y(), q.y()));
    }
  }
  return QRectF(ll, ur); // inverted y-axis
}
