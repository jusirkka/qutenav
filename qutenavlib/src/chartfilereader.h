/* -*- coding: utf-8-unix -*-
 *
 * File: src/chartfilereader.h
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

#include "s57chartoutline.h"
#include "s57object.h"
#include "geoprojection.h"
#include <QtPlugin>

class ChartFileReader {
public:

  virtual const GeoProjection* geoprojection() const = 0;
  virtual GeoProjection* configuredProjection(const QString& path) const = 0;

  virtual S57ChartOutline readOutline(const QString& path,
                                      const GeoProjection* gp) const = 0;

  virtual void readChart(GL::VertexVector& vertices,
                         GL::IndexVector& indices,
                         S57::ObjectVector& objects,
                         const QString& path,
                         const GeoProjection* gp) const = 0;

  virtual bool initializeRead() const {return true;}
  virtual bool cachingSupported() const {return true;}

  const QString& name() const {return m_name;}


  virtual ~ChartFileReader() = default;

  using LevelVector = QVector<quint32>;
  using BGIndexMap = QMap<quint32, WGS84Point>;

  static bool isBGIndex(quint32 index);
  static quint32 numBGIndices(const WGS84Point& sw, const WGS84Point& ne, quint32 lvl);
  static LevelVector bgLevels();
  static BGIndexMap bgIndices(const WGS84Point& sw, const WGS84Point& ne, quint32 lvl);

  static QRectF computeBBox(S57::ElementDataVector &elems,
                            const GL::VertexVector& vertices,
                            const GL::IndexVector& indices);

  static QRectF computeSoundingsBBox(const GL::VertexVector& ps);

  static QPointF computeLineCenter(const S57::ElementDataVector &elems,
                                   const GL::VertexVector& vertices,
                                   const GL::IndexVector& indices);

  static QPointF computeAreaCenterAndBboxes(S57::ElementDataVector& elems,
                                            const GL::VertexVector& vertices,
                                            const GL::IndexVector& indices);

  static void triangulate(S57::ElementDataVector& elems,
                          GL::IndexVector& indices,
                          const GL::VertexVector& vertices,
                          const S57::ElementDataVector& edges);

  struct Edge {
    Edge() = default;
    Edge(const Edge& other) = default;
    Edge& operator =(const Edge& other) = default;
    Edge(quint32 b, quint32 e, quint32 f, quint32 c, bool r = false, bool i = false)
      : begin(b)
      , end(e)
      , first(f)
      , count(c)
      , reversed(r)
      , inner(i) {}

    quint32 begin;
    quint32 end;
    quint32 first;
    quint32 count;
    bool reversed;
    bool inner;
  };
  using EdgeVector = QVector<Edge>;
  using EdgeMap = QMap<quint32, Edge>;

  static S57::ElementDataVector createLineElements(GL::IndexVector& indices,
                                                   GL::VertexVector& vertices,
                                                   const EdgeVector& edges);

  static int addIndices(const Edge& e, GL::IndexVector& indices);

  static void checkCoverage(WGS84Polygon& cov,
                            WGS84Polygon& nocov,
                            WGS84PointVector& ps,
                            const GeoProjection* gp,
                            quint32 scale,
                            quint8* covp = nullptr, quint8* nocovp = nullptr); // percentages

  using PointVector = QVector<QPointF>;

  static void reduceRDP(PointVector& ps, qreal eps);

protected:


  ChartFileReader(const QString& name)
    : m_name(name) {}



  QString m_name;

  static const inline quint32 x0 = 1800;
  static const inline quint32 y0 = 870;
  static const inline quint32 Index_Mask = 0x80000000;

};

class ChartFileReaderFactory {
public:

  ChartFileReader* loadReader(const QStringList& paths) const;
  virtual QString name() const = 0;
  virtual QString displayName() const = 0;
  virtual QStringList filters() const = 0;
  virtual QStringList eulaFilters() const {return QStringList();}

protected:

  virtual void initialize(const QStringList& paths) const = 0;
  virtual ChartFileReader* create() const = 0;

  virtual ~ChartFileReaderFactory() = default;
};

Q_DECLARE_INTERFACE(ChartFileReaderFactory,
                    "net.kvanttiapina.qutenav.ChartFileReaderFactory/1.0")

