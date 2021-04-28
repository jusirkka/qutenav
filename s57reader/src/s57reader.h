/* -*- coding: utf-8-unix -*-
 *
 * File: s57reader/src/s57reader.h
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

#include "chartfilereader.h"

class S57ReaderFactory;
class GeoProjection;


class S57Reader: public ChartFileReader {

  friend class S57ReaderFactory;

public:

  const GeoProjection* geoprojection() const override;
  GeoProjection* configuredProjection(const QString &path) const override;

  S57ChartOutline readOutline(const QString &path, const GeoProjection *gp) const;

  void readChart(GL::VertexVector& vertices,
                 GL::IndexVector& indices,
                 S57::ObjectVector& objects,
                 const QString& path,
                 const GeoProjection* proj) const override;

private:

  using PointVector = QVector<QPointF>;
  using PRegion = QVector<PointVector>;
  using Region = S57ChartOutline::Region;

  struct RawEdge {
    RawEdge() = default;
    RawEdge(const RawEdge& other) = default;
    RawEdge& operator =(const RawEdge& other) = default;
    quint32 begin;
    quint32 end;
    PointVector points;
  };
  using RawEdgeMap = QMap<quint32, RawEdge>;
  using REMIter = RawEdgeMap::const_iterator;

  struct RawEdgeRef {
    quint32 id;
    bool reversed;
    bool inner;
  };
  using RawEdgeRefVector = QVector<RawEdgeRef>;

  struct CovRefs {
    bool cov;
    RawEdgeRefVector refs;
  };

  using CovEdgeRefMap = QMap<quint32, CovRefs>;
  using CovEdgeRefIter = CovEdgeRefMap::const_iterator;
  using PointMap = QMap<quint32, QPointF>;
  using PointMapIter = PointMap::const_iterator;
  using PointRefVector = QVector<quint32>;
  using PointRefMap = QMap<quint32, quint32>;

  struct RawObject {
    quint8 geometry;
    quint16 code;
    RawEdgeRefVector edgeRefs; // lines, areas
    quint32 pointRef; // points
    quint8 refType;
    S57::AttributeMap attributes;
  };

  using RawObjectMap = QMap<quint32, RawObject>;
  using ROMIter = RawObjectMap::const_iterator;



  bool checkCoverage(const PRegion& cov,
                     WGS84Point &sw,
                     WGS84Point &ne,
                     const GeoProjection *gp) const;

  void createCoverage(PRegion& cov, PRegion& nocov,
                      const RawEdgeRefVector& edges, const RawEdgeMap& edgemap,
                      const PointMap& connmap, quint32 mulfac,
                      const GeoProjection* gp) const;

  PointVector addVertices(const RawEdgeRef& e, const RawEdgeMap& edgemap,
                          const PointMap& connmap, quint32 mulfac,
                          const GeoProjection* gp) const;

  Region transformCoverage(PRegion pcov, const GeoProjection* gp, WGS84Point* corners) const;

  static const inline double eps = 1.e-4;
  using IndexVector = QVector<int>;
  void reduceRDP(const PointVector& ps, int first, int last, IndexVector& is) const;

  S57Reader(const QString& name);

  GeoProjection* m_proj;

};

class S57ReaderFactory: public QObject, public ChartFileReaderFactory {

  Q_OBJECT
  Q_PLUGIN_METADATA(IID "net.kvanttiapina.qutenav.ChartFileReaderFactory/1.0")
  Q_INTERFACES(ChartFileReaderFactory)

public:

  QString name() const override;
  QString displayName() const override;
  QStringList filters() const override;

protected:

  void initialize() const override;
  ChartFileReader* create() const override;

};


