/* -*- coding: utf-8-unix -*-
 *
 * File: gshhsreader.cpp
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

#include "shapereader.h"
#include <QFile>
#include <QDataStream>
#include "logging.h"
// #include "gnuplot.h"
#include "chartfilereader.h"



ShapeReader::ShapeReader(const QRectF& box, const GeoProjection* gp)
  : m_box(box)
  , m_proj(gp)
{}


template<typename T> T read_value(QDataStream& stream) {

  T value;
  stream >> value;

  return value;
}

QRectF read_bbox(QDataStream& stream, const GeoProjection* gp) {
  const auto x0 = read_value<double>(stream);
  const auto y0 = read_value<double>(stream);
  const auto x1 = read_value<double>(stream);
  const auto y1 = read_value<double>(stream);

  //  qDebug() << x0 << y0;
  //  qDebug() << x1 << y1;

  return QRectF(gp->fromWGS84(WGS84Point::fromLL(x0, y0)),
                gp->fromWGS84(WGS84Point::fromLL(x1, y1)));
}

QPointF read_vertex(QDataStream& stream, const GeoProjection* gp) {
  const auto x = read_value<double>(stream);
  const auto y = read_value<double>(stream);

  return gp->fromWGS84(WGS84Point::fromLL(x, y));
}

void ShapeReader::read(GL::VertexVector& vertices,
                       GL::IndexVector& indices,
                       GeomAreaVector& geoms,
                       const QString& path) {

  QFile file(path);
  if (!file.open(QFile::ReadOnly)) {
    throw ChartFileError(QString("Cannot open %1 for reading").arg(path));
  }

  QDataStream stream(&file);
  stream.setByteOrder(QDataStream::BigEndian);
  const auto magic = read_value<qint32>(stream);
  if (magic != 9994) {
    throw ChartFileError(QString("%1 is not a proper ESRI shapefile").arg(path));
  }
  stream.skipRawData(5 * 4); // skip 5 unused fields
  auto len2 = read_value<qint32>(stream); // in units of 16 bit words
  stream.setByteOrder(QDataStream::LittleEndian);
  const auto version = read_value<qint32>(stream);
  if (version != 1000) {
    throw ChartFileError(QString("Unsupported version %1 in %2").arg(version).arg(path));
  }
  auto shpType = read_value<qint32>(stream);
  if (shpType != polygonType) {
    throw ChartFileError(QString("Shapetype is not polygon in %1").arg(path));
  }
  stream.skipRawData(8 * 8); // skip bbox
  len2 -= headerLen2;
  // qDebug() << "remaining words" << len2;
  while (len2 > 0) {
    stream.setByteOrder(QDataStream::BigEndian);
    const auto recNum = read_value<qint32>(stream);
    auto contentLen2 = read_value<qint32>(stream);
    len2 -= contentLen2 + 4;
    // qDebug() << "remaining words" << len2;
    stream.setByteOrder(QDataStream::LittleEndian);
    shpType = read_value<qint32>(stream);
    contentLen2 -= 2;
    if (shpType != polygonType) {
      throw ChartFileError(QString("Shapetype %1 in record %2 is not polygon in %3").arg(shpType).arg(recNum).arg(path));
    }
    auto bbox = read_bbox(stream, m_proj);
    contentLen2 -= 16;
    if (!m_box.intersects(bbox)) {
      // qDebug() << "skipping" << recNum;
      stream.skipRawData(contentLen2 * 2);
      continue;
    }
    const auto numParts = read_value<qint32>(stream);
    contentLen2 -= 2;
    if (numParts != 1) {
      throw ChartFileError(QString("Only one polygon per record supported"));
    }
    const auto numPoints = read_value<qint32>(stream);
    // qDebug() << "Parsing record" << recNum << ", number of points" << numPoints;
    contentLen2 -= 2;
    const auto part0 = read_value<qint32>(stream);
    contentLen2 -= 2;
    Q_ASSERT(part0 == 0);

    QPointF prev;
    PointVector tail;
    // corner case: border point as first point causes issues
    for (prev = read_vertex(stream, m_proj);
         boundaryPoint(prev);
         prev = read_vertex(stream, m_proj)) {
      tail.append(prev);
    }
    if (!tail.isEmpty()) {
      tail.append(prev);
    }
    bool in = boxContains(prev);

    IndexVector is;

    XPVector xps;
    const qreal w = m_box.width();
    const qreal h = m_box.height();
    xps.append(XP(XP::Type::Corner, 0, 0));
    xps.append(XP(XP::Type::Corner, w, 0));
    xps.append(XP(XP::Type::Corner, w + h, 0));
    xps.append(XP(XP::Type::Corner, 2 * w + h, 0));

    //    PolygonVector parts;
    //    PointVector part;
    //    part << m_box.bottomLeft() << m_box.bottomRight() << m_box.topRight() << m_box.topLeft();
    //    parts.append(part);
    //    part.clear();

    // Check if one corner is inside polygon -
    // handles the case of all vertices outside of box
    bool cornerIn = false;
    // check if polygon is entirely outside polygon
    const auto prevSize = vertices.size();

    const int startIndex = tail.isEmpty() ? 1 : tail.size();
    for (int i = startIndex; (i < numPoints) || !tail.isEmpty(); i++) {
      QPointF p;
      if (i < numPoints) {
        p = read_vertex(stream, m_proj);
      } else {
        p = tail.takeFirst();
      }
      // part << p;

      if (inoutFlip(prev, p)) cornerIn = !cornerIn;

      if (boxContains(p)) {
        if (!in) {
          in = true;
          const auto p0 = intersection(p, prev);
          // add ref out -> in
          const XP i0(XP::Type::GoingIn, fromVertex(p0), vertices.size() / 2);
          xps.append(i0);
          is.append(vertices.size() / 2);
          vertices << p0.x() << p0.y();
        }
        vertices << p.x() << p.y();
      } else {
        if (in) {
          in = false;
          const auto p0 = intersection(prev, p);
          const auto s0 = fromVertex(p0);
          if (xps.last().s == s0 && xps.last().type != XP::Type::Corner) {
            // Just touches the box border - remove the intersection
            removeTail(vertices, xps.last().index, p0);
            xps.takeLast();
            is.takeLast();
          } else {
            // add ref in -> out
            const XP i0(XP::Type::GoingOut, s0, vertices.size() / 2);
            xps.append(i0);
            is.append(vertices.size() / 2);
            vertices << p0.x() << p0.y();
          }
        } else {
          auto qs = intersections(prev, p);
          if (!qs.isEmpty()) {
            qDebug() << qSetRealNumberPrecision(20) << "TWO INTERSECTIONS" << prev << p << qs;
            Q_ASSERT(qs.size() == 2);
            // add ref out -> in
            const XP i0(XP::Type::GoingIn, fromVertex(qs.first()), vertices.size() / 2);
            xps.append(i0);
            is.append(vertices.size() / 2);
            vertices << qs.first().x() << qs.first().y();
            // add ref in -> out
            const XP i1(XP::Type::GoingOut, fromVertex(qs.last()), vertices.size() / 2);
            xps.append(i1);
            is.append(vertices.size() / 2);
            vertices << qs.last().x() << qs.last().y();
          }
        }
      }
      prev = p;
    }

    //    parts.append(part);
    //    part.clear();
    //    shape::tognuplot(parts, 2 /*recNum*/, false);
    //    parts.clear();

    contentLen2 -= 8 * numPoints;
    Q_ASSERT(contentLen2 == 0);

    if (is.isEmpty() && vertices.size() == prevSize && cornerIn) {
      qDebug() << recNum << ": Polygon covers whole box";
      auto area = createBoxGeometry(vertices, indices);
      GeomArea geom {area, m_box};
      geoms.append(geom);
      continue;
    }

    createAreaGeometries(geoms, vertices, indices, xps, is, prevSize);
  }

  Q_ASSERT(len2 == 0);
}

using Edge = ChartFileReader::Edge;
using EdgeVector = ChartFileReader::EdgeVector;


S57::Geometry::Area* ShapeReader::createBoxGeometry(GL::VertexVector& vertices, GL::IndexVector& indices) const {
  const PointVector corners {
    m_box.topLeft(), m_box.bottomLeft(), m_box.bottomRight(), m_box.topRight()
  };
  for (const QPointF& c: corners) {
    vertices << c.x() << c.y();
  }
  const quint32 first = vertices.size() / 2 - 4;
  Edge edge(first, first, first + 1, 3);
  EdgeVector edges {edge};
  const auto lines = ChartFileReader::createLineElements(indices, vertices, edges);
  S57::ElementDataVector triangles;
  ChartFileReader::triangulate(triangles, indices, vertices, lines);
  const auto center = ChartFileReader::computeAreaCenterAndBboxes(triangles, vertices, indices);
  return new S57::Geometry::Area(S57::ElementDataVector(),
                                 center,
                                 triangles,
                                 0,
                                 true,
                                 m_proj);
}



void ShapeReader::createAreaGeometries(GeomAreaVector& geoms,
                                       GL::VertexVector& vertices,
                                       GL::IndexVector& indices,
                                       XPVector& xps,
                                       IndexVector& is,
                                       quint32 vertexOffset) const {

  if (vertices.size() == static_cast<int>(vertexOffset)) return;

  if (is.isEmpty()) {
    const quint32 first = vertexOffset / 2;
    const quint32 count = vertices.size() / 2 - first - 1;
    Edge edge(first, first, first + 1, count);
    EdgeVector edges {edge};
    auto lines = ChartFileReader::createLineElements(indices, vertices, edges);
    auto box = ChartFileReader::computeBBox(lines, vertices, indices);

    S57::ElementDataVector triangles;
    ChartFileReader::triangulate(triangles, indices, vertices, lines);
    const auto center = ChartFileReader::computeAreaCenterAndBboxes(triangles, vertices, indices);

    auto area = new S57::Geometry::Area(lines,
                                        center,
                                        triangles,
                                        0,
                                        true,
                                        m_proj);
    GeomArea geom {area, box};
    geoms.append(geom);
    return;
  }

  std::sort(xps.begin(), xps.end(), [] (const XP& x1, const XP& x2) {
    return x1.s < x2.s;
  });

  // check consistency
  Q_ASSERT(is.size() % 2 == 0);
  const int I = is.size();
  for (int i = 0; i < I; i++) {
    const auto i1 = is[(i + 1) % I];
    auto it1 = std::find_if(xps.cbegin(), xps.cend(), [i1] (const XP& xp) {
      if (xp.type == XP::Type::Corner) return false;
      return xp.index == i1;
    });
    Q_ASSERT(it1 != xps.cend());
    const auto i0 = is[i];
    auto it0 = std::find_if(xps.cbegin(), xps.cend(), [i0] (const XP& xp) {
      if (xp.type == XP::Type::Corner) return false;
      return xp.index == i0;
    });
    Q_ASSERT(it0 != xps.cend());
    Q_ASSERT(it1->type != it0->type);
  }
  Q_ASSERT(xps.size() == is.size() + 4);
  const int M = xps.size();
  if (M > 4) {
    for (int i = 0; i < M; i++) {
      int j = (i + 1) % M;
      while (xps[j].type == XP::Type::Corner) j = (j + 1) % M;
      Q_ASSERT(xps[j].type != xps[i].type);
    }
  }
  auto it = std::find_if(xps.cbegin(), xps.cend(), [is] (const XP& xp) {
    if (xp.type == XP::Type::Corner || is.isEmpty()) return false;
    return xp.index == is.first();
  });
  if (it != xps.cend()) {
    if (it->type == XP::Type::GoingOut) {
      is.append(is.takeFirst());
    }
  }


  quint32 lastIndex = 0;
  quint32 nextIndex = !is.isEmpty() ? is.first() : 0;
  const quint32 lastShapeIndex = vertices.size() / 2 - 1;
  const quint32 firstShapeIndex = vertexOffset / 2;
  EdgeVector areaEdges;
  EdgeVector lineEdges;
  while (!is.isEmpty()) {
    quint32 i1;
    quint32 i2;
    if (nextIndex == is.first()) {
      i1 = is.takeFirst();
      i2 = is.takeFirst();
    } else {
      IndexVector head;
      while (is.first() != nextIndex) {
        head.append(is.takeFirst());
      }
      i1 = is.takeFirst();
      i2 = is.takeFirst();
      head.append(is);
      is = head;
    }
    if (areaEdges.isEmpty()) {
      lastIndex = i1;
    }
    if (i1 < i2) {
      Edge edge(i1, i2, i1 + 1, i2 - i1 - 1);
      areaEdges.append(edge);
      lineEdges.append(edge);
    } else if (i2 == firstShapeIndex) {
      Edge edge(i1, i2, i1 + 1, lastShapeIndex - i1);
      areaEdges.append(edge);
      lineEdges.append(edge);
    } else if (i1 == lastShapeIndex) {
      Edge edge(i1, i2, firstShapeIndex, i2 - firstShapeIndex);
      areaEdges.append(edge);
      lineEdges.append(edge);
    } else {
      Edge edge1(i1, firstShapeIndex, i1 + 1, lastShapeIndex - i1);
      areaEdges.append(edge1);
      lineEdges.append(edge1);
      Edge edge2(firstShapeIndex, i2, firstShapeIndex + 1, i2 - firstShapeIndex - 1);
      areaEdges.append(edge2);
      lineEdges.append(edge2);
    }

    // add to areaEdges only
    int j = 0;
    while (xps[j].index != i2 || xps[j].type == XP::Type::Corner) j++;
    j = (j + M + 1) % M;
    const auto lastSize = vertices.size();
    while (xps[j].type == XP::Type::Corner) {
      const auto p = toVertex(xps[j].s);
      vertices << p.x() << p.y();
      j = (j + M + 1) % M;
    }
    Edge cornerEdge(i2, xps[j].index, lastSize / 2, (vertices.size() - lastSize) / 2);
    areaEdges.append(cornerEdge);

    if (xps[j].index == lastIndex) {
      // create geom
      const auto poly = ChartFileReader::createLineElements(indices, vertices, areaEdges);
      S57::ElementDataVector triangles;
      ChartFileReader::triangulate(triangles, indices, vertices, poly);
      const auto center = ChartFileReader::computeAreaCenterAndBboxes(triangles, vertices, indices);
      auto lines = ChartFileReader::createLineElements(indices, vertices, lineEdges);
      auto box = ChartFileReader::computeBBox(lines, vertices, indices);
      auto area = new S57::Geometry::Area(lines,
                                          center,
                                          triangles,
                                          0,
                                          true,
                                          m_proj);
      GeomArea geom {area, box};
      geoms.append(geom);

      areaEdges.clear();
      lineEdges.clear();
      if (!is.isEmpty()) {
        nextIndex = is.first();
      }
    } else {
      Q_ASSERT(!is.isEmpty());
      nextIndex = xps[j].index;
    }
  }

}

qreal ShapeReader::fromVertex(const QPointF& p) const {

  const qreal w = m_box.width();
  const qreal h = m_box.height();

  if (p.y() > m_box.bottom() - eps) return p.x() - m_box.left();
  if (p.x() > m_box.right() - eps) return w + m_box.bottom() - p.y();
  if (p.y() < m_box.top() + eps) return 2 * w + h + m_box.left() - p.x();
  if (p.x() < m_box.left() + eps) return 2 * w + h + p.y() - m_box.top();
  Q_ASSERT(false);
  return -1.0;
}

QPointF ShapeReader::toVertex(qreal s) const {
  const qreal w = m_box.width();
  const qreal h = m_box.height();

  if (s < w) return QPointF(m_box.left() + s, m_box.bottom());
  if (s < w + h) return QPointF(m_box.right(), m_box.bottom() + w - s);
  if (s < 2 * w + h) return QPointF(m_box.right() + w + h - s, m_box.top());
  if (s < 2 * w + 2 * h) return QPointF(m_box.left(), m_box.top() + s - 2 * w - h);
  Q_ASSERT(false);
  return QPointF(0., 0.);
}

bool ShapeReader::inoutFlip(const QPointF& s1, const QPointF& s2) const {
  const QPointF& p = m_box.topLeft();
  return ((s1.y() > p.y()) != (s2.y() > p.y())) &&
      (p.x() < (s2.x() - s1.x()) * (p.y() - s1.y()) / (s2.y() - s1.y()) + s1.x());
}

QPointF ShapeReader::intersection(const QPointF& pin, const QPointF& pout) const {
  if (pout.y() > m_box.bottom()) {
    const auto p0 = intersection_top(pin, pout);
    if (p0.x() < m_box.left()) return intersection_left(pin, pout);
    if (p0.x() > m_box.right()) return intersection_right(pin, pout);
    return p0;
  }
  if (pout.y() < m_box.top()) {
    const auto p0 = intersection_bottom(pin, pout);
    if (p0.x() < m_box.left()) return intersection_left(pin, pout);
    if (p0.x() > m_box.right()) return intersection_right(pin, pout);
    return p0;
  }
  if (pout.x() < m_box.left()) return intersection_left(pin, pout);
  return intersection_right(pin, pout);
}

QPointF ShapeReader::intersection(const QPointF& p1, const QPointF& p2, quint8 code) const {
  if (code & TOP) return intersection_top(p1, p2);
  if (code & BOTTOM) return intersection_bottom(p1, p2);
  if (code & LEFT) return intersection_left(p1, p2);
  if (code & RIGHT) return intersection_right(p1, p2);
  Q_ASSERT(false);
  return QPointF(0., 0.);
}

QPointF ShapeReader::intersection_left(const QPointF& p1, const QPointF& p2) const {
  return QPointF(m_box.left(), p1.y() + (m_box.left() - p1.x()) * (p2.y() - p1.y()) / (p2.x() - p1.x()));
}

QPointF ShapeReader::intersection_right(const QPointF& p1, const QPointF& p2) const {
  return QPointF(m_box.right(), p1.y() + (m_box.right() - p1.x()) * (p2.y() - p1.y()) / (p2.x() - p1.x()));
}

QPointF ShapeReader::intersection_top(const QPointF& p1, const QPointF& p2) const {
  return QPointF(p1.x() + (m_box.bottom() - p1.y()) * (p2.x() - p1.x()) / (p2.y() - p1.y()), m_box.bottom());
}

QPointF ShapeReader::intersection_bottom(const QPointF& p1, const QPointF& p2) const {
  return QPointF(p1.x() + (m_box.top() - p1.y()) * (p2.x() - p1.x()) / (p2.y() - p1.y()), m_box.top());
}


bool ShapeReader::boxContains(const QPointF& p) const {
  if (p.y() > m_box.bottom()) return false;
  if (p.y() < m_box.top()) return false;
  if (p.x() > m_box.right()) return false;
  if (p.x() < m_box.left()) return false;
  return true;
}

bool ShapeReader::boundaryPoint(const QPointF& p) const {
  if (!boxContains(p)) return false;
  if (p.y() == m_box.bottom()) return true;
  if (p.y() == m_box.top()) return true;
  if (p.x() == m_box.right()) return true;
  if (p.x() == m_box.left()) return true;
  return false;
}



quint8 ShapeReader::locationCode(const QPointF& v) const {
  quint8 code = 0;
  if (v.y() > m_box.bottom()) {
    code |= TOP;
  } else if (v.y() < m_box.top()) {
    code |= BOTTOM;
  }

  if (v.x() > m_box.right()) {
    code |= RIGHT;
  } else if (v.x() < m_box.left()) {
    code |= LEFT;
  }

  return code;
}


ShapeReader::PointVector ShapeReader::intersections(const QPointF& p1, const QPointF& p2) const {
  QPointF a = p1;
  QPointF b = p2;
  auto codeA = locationCode(p1);
  auto codeB = locationCode(p2);

  while (true) {

    if (!(codeA | codeB)) { // accept
      return PointVector {a, b};
    }

    if (codeA & codeB) { // trivial reject
      return PointVector();

    }

    if (codeA) { // a outside, intersect with clip edge
      a = intersection(a, b, codeA);
      codeA = locationCode(a);

    } else { // b outside
      b = intersection(a, b, codeB);
      codeB = locationCode(b);
    }
  }
  return PointVector();
}

void ShapeReader::removeTail(GL::VertexVector& vertices, quint32 index, const QPointF& p) const {
  quint32 lastIndex = vertices.size() / 2 - 1;
  while (index >= lastIndex) {
    const auto y = vertices.takeLast();
    const auto x = vertices.takeLast();
    qDebug() << "removing" << index << lastIndex << x << y << p.x() << p.y() <<
                QPointF::dotProduct(p, QPointF(x, y));
    lastIndex = vertices.size() / 2 - 1;
  }
}


