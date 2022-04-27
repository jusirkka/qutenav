/* -*- coding: utf-8-unix -*-
 *
 * File: src/s57chart.h
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

#include "types.h"
#include <QObject>
#include "s57object.h"
#include "s52presentation.h"
#include <QMatrix4x4>
#include <QMutex>
#include "chartproxy.h"


class GeoProjection;
class Camera;
class QOpenGLContext;
class QPainter;
namespace KV {class Region;}

inline uint qHash(const QColor& c) {
  return c.rgba();
}


class S57Chart: public QObject {

  Q_OBJECT

public:

  S57Chart(quint32 id, const QString& path);
  void encode(QDataStream& stream);

  void updateModelTransform(const Camera* cam);

  void drawAreas(const Camera* cam, int prio, bool blend = false);
  void drawLineArrays(const Camera* cam, int prio, bool blend = false);
  void drawSegmentArrays(const Camera* cam, int prio);
  void drawLineElems(const Camera* cam, int prio, bool blend = false);
  void drawText(const Camera* cam, int prio);
  void drawRasterSymbols(const Camera* cam, int prio);
  void drawVectorSymbols(const Camera* cam, int prio, bool blend = false);
  void drawVectorPatterns(const Camera* cam);
  void drawRasterPatterns(const Camera* cam);

  const GeoProjection* geoProjection() const {return m_nativeProj;}

  quint32 id() const {return m_id;}
  const QString& path() {return m_path;}

  void updatePaintData(const WGS84PointVector& cover, quint32 scale);
  void updateLookups();

  S57::InfoTypeFull objectInfoFull(const WGS84Point& p, quint32 scale);
  S57::InfoType objectInfo(const WGS84Point& p, quint32 scale);

  void paintIcon(QPainter& painter, quint32 objectIndex) const;

  // mutex inteface
  void lock();
  void unlock();

  GL::ChartProxy* proxy() {return m_proxy;}


  ~S57Chart();

signals:

  void destroyProxy(GL::ChartProxy* proxy);

public slots:

private:

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

  using LocationHash = S57::Object::LocationHash;
  using LocationIterator = S57::Object::LocationIterator;
  using ContourVector = S57::Object::ContourVector;

  using SymbolMap = QHash<SymbolKey, S57::PaintData*>;
  using SymbolIterator = SymbolMap::const_iterator;
  using SymbolMutIterator = SymbolMap::iterator;
  using SymbolPriorityVector = QVector<SymbolMap>;

  using TextColorMap = QHash<QColor, S57::PaintData*>;
  using TextColorIterator = TextColorMap::const_iterator;
  using TextColorMutIterator = TextColorMap::iterator;
  using TextColorPriorityVector = QVector<TextColorMap>;

  qreal scaleFactor(const QRectF& va, quint32 scale) const;

  void findUnderling(S57::Object* overling,
                     const S57::ObjectVector& candidates,
                     const GL::VertexVector& vertices,
                     const GL::IndexVector& indices);

  GeoProjection* m_nativeProj;
  ObjectLookupVector m_lookups;
  LocationHash m_locations;
  ContourVector m_contours;
  PaintPriorityVector m_paintData;
  quint32 m_id;
  QString m_path;

  QMatrix4x4 m_modelMatrix;

  const QVector<quint32> m_infoSkipList;
  const QVector<quint32> m_navaids;
  const quint32 m_light;

  QMutex m_mutex;

  GL::ChartProxy* m_proxy;
};


Q_DECLARE_METATYPE(S57Chart*)

