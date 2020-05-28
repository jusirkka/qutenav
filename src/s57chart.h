#pragma once

#include <GL/gl.h>
#include "types.h"
#include <QObject>
#include "s57object.h"
#include "s52presentation.h"

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
    : m_points(std::move(points)) {}

  const EightFloater& eightFloater() const {return m_points;}

private:

  EightFloater m_points;
};

class S57ChartOutline {

public:

  S57ChartOutline(const QString& path);

  const Extent& extent() const;
  const WGS84Point& reference() const;

private:

  Extent m_extent;
  WGS84Point m_ref;

};


class GeoProjection;

class S57Chart: public QObject {

  Q_OBJECT

public:

  using VertexVector = QVector<GLfloat>;
  using IndexVector = QVector<GLuint>;

  S57Chart(const QString& path,
           const GeoProjection* proj,
           QObject *parent);

  const VertexVector& vertices() const {return m_vertices;}
  const IndexVector& indices() const {return m_indices;}

  const S57::PaintDataVector lines(int prio) const {
    return m_paintData[prio][S57::PaintData::Type::Lines];
  }

  const S57::PaintDataVector triangles(int prio) const {
    return m_paintData[prio][S57::PaintData::Type::Triangles];
  }

  const GeoProjection* geoProjection() const {return m_nativeProj;}

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

  using PaintPriorityVector = QVector<S57::PaintDataVectorMap>;

  GLsizei addIndices(GLuint first, GLuint lower, GLuint upper, bool reversed);
  void triangulate(const S57::ElementDataVector& lelems, S57::ElementDataVector& telems);
  void updatePaintData();


  Extent m_extent;
  GeoProjection* m_nativeProj;
  PolygonVector m_coverage;
  PolygonVector m_nocoverage;
  ObjectLookupVector m_lookups;
  VertexVector m_vertices;
  IndexVector m_indices;
  PaintPriorityVector m_paintData;
};

