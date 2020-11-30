#pragma once

#include <GL/gl.h>
#include "types.h"
#include <QObject>
#include "s57object.h"
#include "s52presentation.h"
#include <QDate>
#include <QMatrix4x4>
#include <QOpenGLBuffer>

//  OSENC V2 record definitions
enum class SencRecordType: quint16 {
  HEADER_SENC_VERSION = 1,
  HEADER_CELL_NAME = 2,
  HEADER_CELL_PUBLISHDATE = 3,
  HEADER_CELL_EDITION = 4,
  HEADER_CELL_UPDATEDATE = 5,
  HEADER_CELL_UPDATE = 6,
  HEADER_CELL_NATIVESCALE = 7,
  HEADER_CELL_SENCCREATEDATE = 8,
  FEATURE_ID_RECORD = 64,
  FEATURE_ATTRIBUTE_RECORD = 65,
  FEATURE_GEOMETRY_RECORD_POINT = 80,
  FEATURE_GEOMETRY_RECORD_LINE = 81,
  FEATURE_GEOMETRY_RECORD_AREA = 82,
  FEATURE_GEOMETRY_RECORD_MULTIPOINT = 83,
  VECTOR_EDGE_NODE_TABLE_RECORD = 96,
  VECTOR_CONNECTED_NODE_TABLE_RECORD = 97,
  CELL_COVR_RECORD = 98,
  CELL_NOCOVR_RECORD = 99,
  CELL_EXTENT_RECORD = 100
};

#pragma pack(push,1)

struct OSENC_Record_Base {
  SencRecordType record_type;
  quint32 record_length;
};

struct OSENC_EXTENT_Record_Payload {
  double extent_sw_lat;
  double extent_sw_lon;
  double extent_nw_lat;
  double extent_nw_lon;
  double extent_ne_lat;
  double extent_ne_lon;
  double extent_se_lat;
  double extent_se_lon;
};

struct OSENC_POINT_ARRAY_Record_Payload {
  uint32_t count;
  float array;
};


struct OSENC_Feature_Identification_Record_Payload {
  uint16_t feature_type_code;
  uint16_t feature_ID;
  uint8_t feature_primitive;
};

struct OSENC_Attribute_Record_Payload {
  uint16_t attribute_type_code;
  uint8_t attribute_value_type;
  char attribute_data;
};

struct OSENC_PointGeometry_Record_Payload {
  double lat;
  double lon;
};

struct OSENC_MultipointGeometry_Record_Payload {
  double extent_s_lat;
  double extent_n_lat;
  double extent_w_lon;
  double extent_e_lon;
  uint32_t point_count;
  uint8_t point_data;
};

struct OSENC_LineGeometry_Record_Payload {
  double extent_s_lat;
  double extent_n_lat;
  double extent_w_lon;
  double extent_e_lon;
  uint32_t edgeVector_count;
  uint8_t edge_data;
};

struct OSENC_AreaGeometry_Record_Payload {
  double extent_s_lat;
  double extent_n_lat;
  double extent_w_lon;
  double extent_e_lon;
  uint32_t contour_count;
  uint32_t triprim_count;
  uint32_t edgeVector_count;
  uint8_t edge_data;
};

#pragma pack(pop)



class Extent {
public:
  using EightFloater = QVector<GLfloat>;

  Extent() = default;
  Extent(EightFloater points)
    : m_points(std::move(points)) {
    for (int i = 0; i < 4; i++) {
      auto p = WGS84Point::fromLL(m_points[2 * i], m_points[2 * i + 1]);
      m_wgs84Points.append(p);
    }
  }

  const EightFloater& eightFloater() const {return m_points;}
  const WGS84PointVector& corners() const {return m_wgs84Points;}

private:

  EightFloater m_points;
  WGS84PointVector m_wgs84Points;
};

class S57ChartOutline {

public:

  S57ChartOutline(const QString& path);

  const Extent& extent() const {return m_extent;}
  const WGS84Point& reference() const {return m_ref;}
  quint32 scale() const {return m_scale;}
  const QDate& published() const {return m_pub;}
  const QDate& modified() const {return m_mod;}

private:

  Extent m_extent;
  WGS84Point m_ref;
  quint32 m_scale;
  QDate m_pub;
  QDate m_mod;

};


class GeoProjection;
class Settings;
class Camera;
class QOpenGLContext;

class S57Chart: public QObject {

  Q_OBJECT

public:

  using VertexVector = QVector<GLfloat>;
  using IndexVector = QVector<GLuint>;

  S57Chart(quint32 id, const QString& path, const GeoProjection* proj);

  void drawAreas(int prio);
  void drawSolidLines(int prio);
  void drawDashedLines();
  void setTransform(const Camera* cam);

  const GeoProjection* geoProjection() const {return m_nativeProj;}

  quint32 id() const {return m_id;}

  void updatePaintData(const QRectF& viewArea, quint32 scale);
  void finalizePaintData();

signals:

public slots:

private:

  using Polygon = QVector<float>; // two floats per point
  using PolygonVector = QVector<Polygon>;

  struct ObjectLookup {
    ObjectLookup(const S57::Object* obj, S52::Lookup* lup)
      : object(obj)
      , lookup(lup) {}

    ObjectLookup() = default;

    const S57::Object* object;
    S52::Lookup* lookup;
  };

  using ObjectLookupVector = QVector<ObjectLookup>;

  using PaintPriorityVector = QVector<S57::PaintDataMap>;

  static void triangulate(const S57::ElementDataVector& lelems,
                          S57::ElementDataVector& telems,
                          const VertexVector& vertices,
                          IndexVector& indices);

  Extent m_extent;
  GeoProjection* m_nativeProj;
  PolygonVector m_coverage;
  PolygonVector m_nocoverage;
  ObjectLookupVector m_lookups;
  PaintPriorityVector m_paintData;
  PaintPriorityVector m_updatedPaintData;
  VertexVector m_updatedVertices;
  quint32 m_id;
  Settings* m_settings;
  QMatrix4x4 m_pvm;
  QOpenGLBuffer m_coordBuffer;
  QOpenGLBuffer m_indexBuffer;
  GLsizei m_staticVertexOffset;
  GLsizei m_staticElemOffset;

};

