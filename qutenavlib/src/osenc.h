/* -*- coding: utf-8-unix -*-
 *
 * File: src/osenc.h
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
#pragma once

#include <QString>
#include "s57chartoutline.h"
#include "s57object.h"
#include <QIODevice>

class GeoProjection;

class Osenc {


public:

  Osenc() = default;

  GeoProjection* configuredProjection(QIODevice* device, const QString& clsName) const;

  S57ChartOutline readOutline(QIODevice* device, const GeoProjection* proj) const;

  void readChart(GL::VertexVector& vertices,
                 GL::IndexVector& indices,
                 S57::ObjectVector& objects,
                 QIODevice* device,
                 const GeoProjection* proj) const;


private:

  QPointF computeAreaCenterAndBboxes(S57::ElementDataVector& elems,
                                     const GL::VertexVector& vertices,
                                     GLsizei offset) const;

  struct RawEdge {
    quint32 first;
    quint32 count;
  };

  using RawEdgeMap = QMap<quint32, RawEdge>;
  using REMIter = RawEdgeMap::const_iterator;

  using PointRefMap = QMap<quint32, quint32>;

  struct TrianglePatch {
    TrianglePatch()
      : mode(GL_TRIANGLES) {}
    GLenum mode;
    GL::VertexVector vertices;
  };

  using TrianglePatchVector = QVector<TrianglePatch>;

  static const int blockSize = 3000;

  struct RawEdgeRef {
    quint32 begin;
    quint32 end;
    quint32 index;
    bool reversed;
  };

  using RawEdgeRefVector = QVector<RawEdgeRef>;

  struct ObjectWrapper {
    explicit ObjectWrapper(S57::Object* obj = nullptr)
      : object(obj)
      , edgeRefs()
      , triangles()
      , geom(S57::Geometry::Type::Meta) {}


    S57::Object* object;
    RawEdgeRefVector edgeRefs; // lines, areas
    TrianglePatchVector triangles;
    S57::Geometry::Type geom;
  };

  using ObjectWrapperVector = QVector<ObjectWrapper>;

  enum class AttributeRecType: quint8 {
    Integer = 0,
    Real = 2,
    String = 4
  };

  static const inline QVector<quint8> AllAttrTypes {0, 2, 4};

};


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
  HEADER_CELL_SOUNDINGDATUM = 9,
  FEATURE_ID_RECORD = 64,
  FEATURE_ATTRIBUTE_RECORD = 65,
  FEATURE_GEOMETRY_RECORD_POINT = 80,
  FEATURE_GEOMETRY_RECORD_LINE = 81,
  FEATURE_GEOMETRY_RECORD_AREA = 82,
  FEATURE_GEOMETRY_RECORD_MULTIPOINT = 83,
  FEATURE_GEOMETRY_RECORD_AREA_EXT = 84,
  VECTOR_EDGE_NODE_TABLE_EXT_RECORD = 85,
  VECTOR_CONNECTED_NODE_TABLE_EXT_RECORD = 86,
  VECTOR_EDGE_NODE_TABLE_RECORD = 96,
  VECTOR_CONNECTED_NODE_TABLE_RECORD = 97,
  CELL_COVR_RECORD = 98,
  CELL_NOCOVR_RECORD = 99,
  CELL_EXTENT_RECORD = 100,
  CELL_TXTDSC_INFO_FILE_RECORD = 101
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
  char point_data;
};

struct OSENC_LineGeometry_Record_Payload {
  double extent_s_lat;
  double extent_n_lat;
  double extent_w_lon;
  double extent_e_lon;
  uint32_t edgeVector_count;
  char edge_data;
};

struct OSENC_AreaGeometry_Record_Payload {
  double extent_s_lat;
  double extent_n_lat;
  double extent_w_lon;
  double extent_e_lon;
  uint32_t contour_count;
  uint32_t triprim_count;
  uint32_t edgeVector_count;
  char edge_data;
};

#pragma pack(pop)

