/* -*- coding: utf-8-unix -*-
 *
 * File: src/chartfilereader.cpp
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
#include "chartfilereader.h"
#include "triangulator.h"
#include "logging.h"
#include <QVector3D>


ChartFileReader* ChartFileReaderFactory::loadReader(const QStringList& paths) const {
  try {
    initialize(paths);
    return create();
  } catch (ChartFileError& e) {
    qCWarning(CENC) << e.msg();
    return nullptr;
  }
}


bool ChartFileReader::isBGIndex(quint32 index) {
  return (index & Index_Mask) != 0;
}


quint32 ChartFileReader::numBGIndices(const WGS84Point& sw, const WGS84Point& ne, quint32 lvl) {

  // handle dateline
  if (ne.lng() < sw.lng()) {
    const auto ne1 = WGS84Point::fromLL(180., ne.lat());
    const auto sw1 = WGS84Point::fromLL(-180., sw.lat());
    return numBGIndices(sw, ne1, lvl) + numBGIndices(sw1, ne, lvl);
  }

  auto ix1 = static_cast<qint32>(std::floor((10. * sw.lng() + x0) / 2 / lvl));
  ix1 = std::min(static_cast<qint32>(x0 / lvl - 1), std::max(0, ix1));

  auto iy1 = static_cast<qint32>(std::floor((10 * sw.lat() + y0) / 2 / lvl));
  iy1 = std::min(static_cast<qint32>(y0 / lvl - 1), std::max(0, iy1));

  auto ix2 = static_cast<qint32>(std::floor((10. * ne.lng() + x0) / 2 / lvl));
  ix2 = std::min(static_cast<qint32>(x0 / lvl - 1), std::max(0, ix2));

  auto iy2 = static_cast<qint32>(std::floor((10 * ne.lat() + y0) / 2 / lvl));
  iy2 = std::min(static_cast<qint32>(y0 / lvl - 1), std::max(0, iy2));

  Q_ASSERT(ix2 >= ix1 && iy2 >= iy1);

  return (ix2 - ix1 + 1) * (iy2 - iy1 + 1);

}

ChartFileReader::LevelVector ChartFileReader::bgLevels() {
  LevelVector levels {3, 10, 30};
  return levels;
}

ChartFileReader::BGIndexMap ChartFileReader::bgIndices(const WGS84Point& sw, const WGS84Point& ne, quint32 D) {
  // handle dateline
  if (ne.lng() < sw.lng()) {
    const auto ne1 = WGS84Point::fromLL(180., ne.lat());
    const auto sw1 = WGS84Point::fromLL(-180., sw.lat());
    BGIndexMap r = bgIndices(sw, ne1, D);
    const BGIndexMap r2 = bgIndices(sw1, ne, D);
    for (auto it = r2.cbegin(); it != r2.cend(); ++it) {
      r[it.key()] = it.value();
    }
    return r;
  }

  auto ix1 = static_cast<qint32>(std::floor((10. * sw.lng() + x0) / 2 / D));
  ix1 = std::min(static_cast<qint32>(x0 / D - 1), std::max(0, ix1));

  auto iy1 = static_cast<qint32>(std::floor((10 * sw.lat() + y0) / 2 / D));
  iy1 = std::min(static_cast<qint32>(y0 / D - 1), std::max(0, iy1));

  auto ix2 = static_cast<qint32>(std::floor((10. * ne.lng() + x0) / 2 / D));
  ix2 = std::min(static_cast<qint32>(x0 / D - 1), std::max(0, ix2));

  auto iy2 = static_cast<qint32>(std::floor((10 * ne.lat() + y0) / 2 / D));
  iy2 = std::min(static_cast<qint32>(y0 / D - 1), std::max(0, iy2));


  BGIndexMap indices;

  for (qint32 ix = ix1; ix <= ix2; ++ix) {
    for (qint32 iy = iy1; iy <= iy2; ++iy) {

      quint32 bits = iy * x0 / D + ix;
      bits <<= 2;

      const quint32 levelBits = D == 3 ? 1 : D == 10 ? 2 : 3;

      const qint32 xc = D * (2 * ix + 1) - x0;
      const qint32 yc = D * (2 * iy + 1) - y0;

      indices[Index_Mask | bits | levelBits] = WGS84Point::fromLL(.1 * xc, .1 * yc);
    }
  }

  return indices;
}


static void maxbox(QPointF& ll, QPointF& ur, qreal x, qreal y) {
  ll.setX(qMin(ll.x(), x));
  ll.setY(qMin(ll.y(), y));
  ur.setX(qMax(ur.x(), x));
  ur.setY(qMax(ur.y(), y));
}

QRectF ChartFileReader::computeBBox(S57::ElementDataVector &elems,
                                    const GL::VertexVector& vertices,
                                    const GL::IndexVector& indices) {

  QRectF ret;

  auto points = reinterpret_cast<const glm::vec2*>(vertices.constData());
  for (S57::ElementData& elem: elems) {
    QPointF ll(1.e15, 1.e15);
    QPointF ur(-1.e15, -1.e15);
    // assuming chart originated lines
    const int first = elem.offset / sizeof(GLuint);
    for (uint i = 0; i < elem.count; i++) {
      const glm::vec2 q = points[indices[first + i]];
      maxbox(ll, ur, q.x, q.y);
    }
    auto box = QRectF(ll, ur); // inverted y-axis
    // ensure valid bbox
    if (box.width() == 0.) {
      box.setLeft(box.left() - 1.);
      box.setWidth(2.);
    }
    if (box.height() == 0.) {
      box.setTop(box.top() - 1.);
      box.setHeight(2.);
    }
    elem.bbox = box;
    ret |= elem.bbox;
  }
  return ret;
}

QRectF ChartFileReader::computeSoundingsBBox(const GL::VertexVector &ps) {
  QPointF ur(-1.e15, -1.e15);
  QPointF ll(1.e15, 1.e15);
  for (int i = 0; i < ps.size() / 3; i++) {
    const QPointF q(ps[3 * i], ps[3 * i + 1]);
    maxbox(ll, ur, q.x(), q.y());
  }
  if (ll == ur) {
    const qreal res = 20; // 20 meters
    return QRectF(ll - .5 * QPointF(res, res), QSizeF(res, res));
  }
  return QRectF(ll, ur);
}


QPointF ChartFileReader::computeLineCenter(const S57::ElementDataVector &elems,
                                           const GL::VertexVector& vertices,
                                           const GL::IndexVector& indices) {
  int first = elems[0].offset / sizeof(GLuint) + 1; // account adjacency
  int last = first + elems[0].count - 3; // account adjacency
  if (elems.size() > 1 || indices[first] == indices[last]) {
    // several lines or closed loops: compute center of gravity
    QPointF s(0, 0);
    int n = 0;
    for (auto& elem: elems) {
      first = elem.offset / sizeof(GLuint) + 1; // account adjacency
      last = first + elem.count - 3; // account adjacency
      for (int i = first; i <= last; i++) {
        const int index = indices[i];
        s.rx() += vertices[2 * index + 0];
        s.ry() += vertices[2 * index + 1];
      }
      n += elem.count - 2;
    }
    return s / n;
  }
  // single open line: compute mid point of running length
  QVector<float> lengths;
  float len = 0;
  for (int i = first; i < last; i++) {
    const int i1 = indices[i + 1];
    const int i0 = indices[i];
    const QPointF d(vertices[2 * i1] - vertices[2 * i0],
                    vertices[2 * i1 + 1] - vertices[2 * i0 + 1]);
    lengths.append(sqrt(QPointF::dotProduct(d, d)));
    len += lengths.last();
  }
  const float halfLen = len / 2;
  len = 0;
  int i = 0;
  while (i < lengths.size() && len < halfLen) {
    len += lengths[i];
    i++;
  }
  const int i1 = indices[first + i];
  const int i0 = indices[first + i - 1];
  const QPointF p1(vertices[2 * i1], vertices[2 * i1 + 1]);
  const QPointF p0(vertices[2 * i0], vertices[2 * i0 + 1]);
  return p0 + (len - halfLen) / lengths[i - 1] * (p1 - p0);

}

QPointF ChartFileReader::computeAreaCenterAndBboxes(S57::ElementDataVector &elems,
                                                    const GL::VertexVector& vertices,
                                                    const GL::IndexVector& indices) {
  float area = 0;
  QPointF s(0, 0);
  for (S57::ElementData& elem: elems) {

    QPointF ll(1.e15, 1.e15);
    QPointF ur(-1.e15, -1.e15);

    if (elem.mode == GL_TRIANGLES) {
      int first = elem.offset / sizeof(GLuint);
      for (uint i = 0; i < elem.count / 3; i++) {
        const QPointF p0(vertices[2 * indices[first + 3 * i + 0] + 0],
                         vertices[2 * indices[first + 3 * i + 0] + 1]);
        const QPointF p1(vertices[2 * indices[first + 3 * i + 1] + 0],
                         vertices[2 * indices[first + 3 * i + 1] + 1]);
        const QPointF p2(vertices[2 * indices[first + 3 * i + 2] + 0],
                         vertices[2 * indices[first + 3 * i + 2] + 1]);

        maxbox(ll, ur, p0.x(), p0.y());
        maxbox(ll, ur, p1.x(), p1.y());
        maxbox(ll, ur, p2.x(), p2.y());

        const float da = std::abs((p1.x() - p0.x()) * (p2.y() - p0.y()) - (p2.x() - p0.x()) * (p1.y() - p0.y()));
        area += da;
        s.rx() += da / 3. * (p0.x() + p1.x() + p2.x());
        s.ry() += da / 3. * (p0.y() + p1.y() + p2.y());
      }
    } else {
      Q_ASSERT_X(false, "computeAreaCenterAndBboxes", "Unknown primitive");
    }

    elem.bbox = QRectF(ll, ur);
  }

  return s / area;
}



void ChartFileReader::triangulate(S57::ElementDataVector &elems,
                                  GL::IndexVector &indices,
                                  const GL::VertexVector &vertices,
                                  const S57::ElementDataVector& edges) {


  if (edges.isEmpty()) return;

  Triangulator tri(vertices, indices);
  int first = edges.first().offset / sizeof(GLuint) + 1;
  int count = edges.first().count - 3;
  // skip open ended linestrings
  if (indices[first] != indices[first + count]) {
    qCWarning(CENC) << "Cannot triangulate";
    return;
  }
  tri.addPolygon(first, count);

  for (int i = 1; i < edges.size(); i++) {
    first = edges[i].offset / sizeof(GLuint) + 1;
    count = edges[i].count - 3;
    // skip open ended linestrings
    if (indices[first] != indices[first + count]) {
      qCDebug(CENC) << "TRIANGULATE: skipping";
      continue;
    }
    tri.addHole(first, count);
  }

  auto triangles = tri.triangulate();

  S57::ElementData e(GL_TRIANGLES, indices.size() * sizeof(GLuint), triangles.size());
  elems.append(e);

  indices.append(triangles);
}

int ChartFileReader::addIndices(const Edge& e, GL::IndexVector& indices) {
  const int N = e.count;
  if (!e.reversed) {
    indices << e.begin;
    for (int i = 0; i < N; i++) {
      indices << e.first + i;
    }
  } else {
    indices << e.end;
    for (int i = 0; i < N; i++) {
      indices << e.first + e.count - 1 - i;
    }
  }
  return e.count + 1;
}

static int addAdjacent(int ep, int nbor, GL::VertexVector& vertices) {
  const float x1 = vertices[2 * ep];
  const float y1 = vertices[2 * ep + 1];
  const float x2 = vertices[2 * nbor];
  const float y2 = vertices[2 * nbor + 1];
  vertices << 2 * x1 - x2 << 2 * y1 - y2;

  return (vertices.size() - 1) / 2;
}


S57::ElementDataVector ChartFileReader::createLineElements(GL::IndexVector &indices,
                                                           GL::VertexVector &vertices,
                                                           const EdgeVector &edges) {

  auto getBeginPoint = [edges] (int i) {
    return edges[i].reversed ? edges[i].end : edges[i].begin;
  };

  auto getEndPoint = [edges] (int i) {
    return edges[i].reversed ? edges[i].begin : edges[i].end;
  };

  S57::ElementDataVector elems;

  for (int i = 0; i < edges.size();) {
    S57::ElementData e;
    e.mode = GL_LINE_STRIP_ADJACENCY_EXT;
    e.offset = indices.size() * sizeof(GLuint);
    indices.append(0); // dummy index to account adjacency
    e.count = 1;
    auto start = getBeginPoint(i);
    auto prevlast = start;
    while (i < edges.size() && prevlast == getBeginPoint(i)) {
      // qCDebug(CENC) << "Edge" << edges[i].begin << edges[i].end << edges[i].reversed;
      e.count += addIndices(edges[i], indices);
      prevlast = getEndPoint(i);
      i++;
    }
    // finalize current element
    const int adj = e.offset / sizeof(GLuint);
    if (prevlast == start) {
      // polygon
      // qCDebug(CENC) << "polygon" << elems.size() << i << edges.size();
      indices[adj] = indices.last(); // prev
      indices.append(indices[adj + 1]); // close polygon
      indices.append(indices[adj + 2]); // adjacent = next of first
    } else {
      // line string
      // qCDebug(CENC) << "linestring" << elems.size() << i << edges.size();
      auto prev = indices.last();
      indices.append(getEndPoint(i - 1));
      indices.append(addAdjacent(indices.last(), prev, vertices));
      indices[adj] = addAdjacent(indices[adj + 1], indices[adj + 2], vertices);
    }
    e.count += 2;
    elems.append(e);
  }

  return elems;
}


using PointVector = ChartFileReader::PointVector;
using PPolygon = QVector<PointVector>;

static float area(const PointVector& ps) {
  float sum = 0;
  const int n = ps.size();
  for (int k = 0; k < n; k++) {
    const QPointF p0 = ps[k];
    const QPointF p1 = ps[(k + 1) % n];
    sum += p0.x() * p1.y() - p0.y() * p1.x();
  }
  return .5 * sum;
}


void ChartFileReader::checkCoverage(WGS84Polygon& cov,
                                    WGS84Polygon& nocov,
                                    WGS84PointVector& cs,
                                    const GeoProjection* gp,
                                    quint32 scale,
                                    quint8* covr, quint8* nocovr) {


  auto fromwgs84 = [gp, scale] (const WGS84Polygon& poly, PPolygon& out) {
    for (const WGS84PointVector& vs: poly) {
      QPointF ll(1.e15, 1.e15);
      QPointF ur(-1.e15, -1.e15);
      PointVector ps;
      for (auto v: vs) {
        const QPointF p = gp->fromWGS84(v);
        maxbox(ll, ur, p.x(), p.y());
        ps << p;
      }
      const WGS84Point sw = gp->toWGS84(ll);
      const WGS84Point nw = gp->toWGS84(QPointF(ll.x(), ur.y()));
      // 5 mm coarseness at nominal scale
      auto eps = 0.005 * scale * (ur.y() - ll.y()) / (nw - sw).meters();
      if (ps.last() != ps.first()) {
        qCDebug(CENC) << "Closing polygon";
        ps << ps.first();
      }
      reduceRDP(ps, eps);
      out.append(ps);
    }
  };

  PPolygon covp;
  fromwgs84(cov, covp);

  PPolygon nocovp;
  fromwgs84(nocov, nocovp);

  // compute bbox from coverage polygons
  QPointF ll(1.e15, 1.e15);
  QPointF ur(-1.e15, -1.e15);

  for (const PointVector& ps: covp) {
    for (const QPointF& p: ps) {
      maxbox(ll, ur, p.x(), p.y());
    }
  }

  cs.clear();
  cs << gp->toWGS84(ll);
  cs << gp->toWGS84(ur);

  const float A = (ur.y() - ll.y()) * (ur.x() - ll.x());

  float totcov = 0;
  for (const PointVector& ps: covp) {
    totcov += std::abs(area(ps) / A);
  }

  float totnocov = 0;
  for (const PointVector& ps: nocovp) {
    totnocov += std::abs(area(ps) / A);
  }

  qCDebug(CENC) << "coverage" << totcov << totnocov;

  auto towgs84 = [gp] (const PPolygon& poly, WGS84Polygon& out, bool doit) {
    out.clear();
    if (doit) {
      for (const PointVector& ps: poly) {
        WGS84PointVector vs;
        for (const QPointF& p: ps) vs << gp->toWGS84(p);
        out.append(vs);
      }
    }
  };

  towgs84(covp, cov, true);
  towgs84(nocovp, nocov, true);

  if (covr != nullptr) {
    *covr = static_cast<quint8>(std::round(100 * totcov));
  }
  if (nocovr != nullptr) {
    *nocovr = static_cast<quint8>(std::round(100 * totnocov));
  }
}

static qreal squaredPointSegmentDistance(const QPointF& pt, const QPointF& seg) {
  static const qreal z2 = 1.e-16;

  const auto p2 = QPointF::dotProduct(seg, seg);
  const auto q2 = QPointF::dotProduct(pt, pt);

  // distance to a point
  if (p2 < z2) return q2;

  const qreal t = QPointF::dotProduct(pt, seg) / p2;

  if (t < 0.) return q2;
  if (t < 1.) return q2 - p2 * t * t;
  return q2 + p2 * (1. - 2. * t);
}

using IndexVector = QVector<int>;

// https://en.wikipedia.org/wiki/Ramer%E2%80%93Douglas%E2%80%93Peucker_algorithm
static void rdpReduceInner(const PointVector& ps, int first, int last, IndexVector& is, qreal eps2) {
  // Find the point with the maximum distance
  double d2max = 0;
  int index = 0;

  const QPointF p = ps[last] - ps[first];

  for (int i = first + 1; i < last; ++i) {
    const QPointF q = ps[i] - ps[first];

    const auto d2 = squaredPointSegmentDistance(q, p);
    if (d2 > d2max) {
      index = i;
      d2max = d2;
    }
  }

  // If max distance is greater than epsilon, recursively simplify
  if (d2max > eps2) {
    is << index;
    rdpReduceInner(ps, first, index, is, eps2);
    rdpReduceInner(ps, index, last, is, eps2);
  }
}

static void rdpReduce(PointVector& ps, qreal eps) {
  if (ps.size() < 3) return;
  IndexVector indices;
  // qCDebug(CENC) << "RDP coarseness" << eps;
  rdpReduceInner(ps, 0, ps.size() - 1, indices, eps * eps);
  if (indices.size() < ps.size() - 2) {
    // qCDebug(CENC) << "RDP reduction" << ps.size() << "->" << indices.size() + 2;
    PointVector out;
    std::sort(indices.begin(), indices.end());
    out << ps.first();
    for (int i: indices) {
      out << ps[i];
    }
    out << ps.last();

    ps = out;
  }
}

void ChartFileReader::reduceRDP(PointVector& ps, qreal eps) {

  if (ps.size() > 5) {
    auto qs = ps;
    rdpReduce(qs, eps);
    for (int cnt = 0; qs.size() < 3 && cnt < 20; ++cnt) {
      // qCWarning(CENC) << "Too much RDP reduction" << ps.size() << "->" << qs.size();
      eps /= 2;
      qs = ps;
      rdpReduce(qs, eps);
    }
    if (qs.size() >= 4) {
      ps = qs;
    }
  }
}
