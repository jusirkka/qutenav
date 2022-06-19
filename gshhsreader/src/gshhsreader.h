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

#include "chartfilereader.h"

class GSHHSReaderFactory;
class GeoProjection;


class GSHHSReader: public ChartFileReader {

  friend class GSHHSReaderFactory;

public:

  static WGS84Point index2Coord(quint32 index);
  static quint32 index2Level(quint32 index);
  static quint32 coord2Index(const WGS84Point& p, quint32 lvl);

  const GeoProjection* geoprojection() const override;
  GeoProjection* configuredProjection(const QString &path) const override;

  S57ChartOutline readOutline(const QString &path, const GeoProjection *gp) const override;

  void readChart(GL::VertexVector& vertices,
                 GL::IndexVector& indices,
                 S57::ObjectVector& objects,
                 const QString& path,
                 const GeoProjection* proj) const override;

  ~GSHHSReader();

private:

  static const inline int polygonType = 5;
  static const inline int headerLen2 = 50; // Length of header in units of 16 bit words

  static quint32 parsePath(const QString& path);

  void readShapeFile(GL::VertexVector& vertices,
                     GL::IndexVector& indices,
                     S57::ObjectVector& objects,
                     const QString& path,
                     const GeoProjection* proj) const;

  QPointF toVertex(qreal s);

  GSHHSReader(const QString& name);

  GeoProjection* m_proj;

  struct Vertex {
    bool onEdge;
    qreal s;
    qreal t;
  };

  using VertexVector = QVector<Vertex>;

  VertexVector crossings(const QPointF& p1, const QPointF& p2);

  QRectF m_box;
};

class GSHHSReaderFactory: public QObject, public ChartFileReaderFactory {

  Q_OBJECT
  Q_PLUGIN_METADATA(IID "net.kvanttiapina.qutenav.ChartFileReaderFactory/1.0")
  Q_INTERFACES(ChartFileReaderFactory)

public:

  QString name() const override;
  QString displayName() const override;
  QStringList filters() const override;

protected:

  void initialize(const QStringList&) const override;
  ChartFileReader* create() const override;

};

namespace GSHHG {

class ShapeFiles {
public:

  static ShapeFiles* instance();
  QString path(int index, int level) const;

  void init(const QStringList& locs);

private:

  ShapeFiles() = default;
  ShapeFiles(const ShapeFiles&) = default;
  ShapeFiles& operator =(const ShapeFiles&) = default;

  QMap<int, QStringList> m_paths;

};

}


