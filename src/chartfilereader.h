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

  const QString& name() const {return m_name;}


  virtual ~ChartFileReader() = default;

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


protected:

  ChartFileReader(const QString& name)
    : m_name(name) {}



  QString m_name;

};

class ChartFileReaderFactory {
public:

  ChartFileReader* loadReader() const;
  virtual QString name() const = 0;
  virtual QString displayName() const = 0;
  virtual QStringList filters() const = 0;

protected:

  virtual void initialize() const = 0;
  virtual ChartFileReader* create() const = 0;

  virtual ~ChartFileReaderFactory() = default;
};

Q_DECLARE_INTERFACE(ChartFileReaderFactory,
                    "net.kvanttiapina.qopencpn.ChartFileReaderFactory/1.0")
