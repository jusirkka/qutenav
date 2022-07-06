/* -*- coding: utf-8-unix -*-
 *
 * File: gsshsreader.h
 *
 * Copyright (C) 2022 Jukka Sirkka
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

#include <QRectF>
#include "types.h"
#include "s57object.h"

class GeoProjection;

class ShapeReader {

public:

  ShapeReader(const WGS84Point& sw, const WGS84Point& ne, const GeoProjection* gp);

  struct GeomArea {
    S57::Geometry::Area* area;
    QRectF box;
  };

  using GeomAreaVector = QVector<GeomArea>;

  void read(GL::VertexVector& vertices,
            GL::IndexVector& indices,
            GeomAreaVector& geoms,
            const QString& path);

  ~ShapeReader() = default;

  S57::Geometry::Area* createBoxGeometry(GL::VertexVector& vertices,
                                         GL::IndexVector& indices) const;
  const QRectF& bbox() const {return m_box;}


private:

  static const inline int polygonType = 5;
  static const inline int headerLen2 = 50; // Length of header in units of 16 bit words

  static const inline double eps = 1.e-6;

  static const inline quint8 LEFT = 1;
  static const inline quint8 RIGHT = 2;
  static const inline quint8 BOTTOM = 4;
  static const inline quint8 TOP = 8;


  using PointVector = QVector<QPointF>;
  using PolygonVector = QVector<PointVector>;
  using IndexVector = QVector<quint32>;

  qreal fromVertex(const QPointF& p) const;
  QPointF toVertex(qreal s) const;
  QPointF intersection(const QPointF& pin, const QPointF& pout) const;
  QPointF intersection(const QPointF& p1, const QPointF& p2, quint8 code) const;
  QPointF intersection_top(const QPointF& pin, const QPointF& pout) const;
  QPointF intersection_bottom(const QPointF& pin, const QPointF& pout) const;
  QPointF intersection_left(const QPointF& pin, const QPointF& pout) const;
  QPointF intersection_right(const QPointF& pin, const QPointF& pout) const;
  PointVector intersections(const QPointF& pout1, const QPointF& pout2) const;
  bool inoutFlip(const QPointF& s1, const QPointF& s2) const;
  bool boxContains(const QPointF& p) const;
  quint8 locationCode(const QPointF& p) const;
  bool boundaryPoint(const QPointF& p) const;
  bool extentIntersects(const QRectF& r) const;

  void removeTail(GL::VertexVector& vertices, quint32 index, const QPointF& p) const;

  struct XP {
    enum class Type {Corner, GoingIn, GoingOut};
    Type type;
    qreal s;
    quint32 index;

    XP(Type t, qreal u, quint32 i)
      : type(t)
      , s(u)
      , index(i) {}
    XP() = default;
  };

  using XPVector = QVector<XP>;

  void createAreaGeometries(GeomAreaVector& geoms,
                            GL::VertexVector& vertices,
                            GL::IndexVector& indices,
                            XPVector& xps,
                            IndexVector& is,
                            quint32 vertexOffset) const;

  const WGS84Point m_sw;
  const WGS84Point m_ne;
  const QRectF m_box;
  const GeoProjection* m_proj;
};

