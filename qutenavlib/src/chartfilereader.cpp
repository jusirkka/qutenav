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


ChartFileReader* ChartFileReaderFactory::loadReader(const QStringList& paths) const {
  try {
    initialize(paths);
    return create();
  } catch (ChartFileError& e) {
    qCWarning(CENC) << e.msg();
    return nullptr;
  }
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

  S57::ElementData e;
  e.mode = GL_TRIANGLES;
  e.count = triangles.size();
  e.offset = indices.size() * sizeof(GLuint);
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


