#include "chartfilereader.h"
#include "triangulator.h"


ChartFileReader* ChartFileReaderFactory::loadReader() const {
  try {
    initialize();
    return create();
  } catch (ChartFileError& e) {
    qWarning() << e.msg();
    return nullptr;
  }
}

QRectF ChartFileReader::computeBBox(S57::ElementDataVector &elems,
                                    const GL::VertexVector& vertices,
                                    const GL::IndexVector& indices) {

  QRectF ret;

  auto points = reinterpret_cast<const glm::vec2*>(vertices.constData());
  for (S57::ElementData& elem: elems) {
    QPointF ur(-1.e15, -1.e15);
    QPointF ll(1.e15, 1.e15);
    // assuming chart originated lines
    const int first = elem.offset / sizeof(GLuint);
    for (uint i = 0; i < elem.count; i++) {
      const glm::vec2 q = points[indices[first + i]];
      ur.setX(qMax(ur.x(), static_cast<qreal>(q.x)));
      ur.setY(qMax(ur.y(), static_cast<qreal>(q.y)));
      ll.setX(qMin(ll.x(), static_cast<qreal>(q.x)));
      ll.setY(qMin(ll.y(), static_cast<qreal>(q.y)));
    }
    elem.bbox = QRectF(ll, ur); // inverted y-axis
    ret |= elem.bbox;
  }
  return ret;
}

QRectF ChartFileReader::computeSoundingsBBox(const GL::VertexVector &ps) {
  QPointF ur(-1.e15, -1.e15);
  QPointF ll(1.e15, 1.e15);
  for (int i = 0; i < ps.size() / 3; i++) {
    const QPointF q(ps[3 * i], ps[3 * i + 1]);
    ur.setX(qMax(ur.x(), q.x()));
    ur.setY(qMax(ur.y(), q.y()));
    ll.setX(qMin(ll.x(), q.x()));
    ll.setY(qMin(ll.y(), q.y()));
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

QPointF ChartFileReader::computeAreaCenter(const S57::ElementDataVector &elems,
                                           const GL::VertexVector& vertices,
                                           const GL::IndexVector& indices) {
  float area = 0;
  QPointF s(0, 0);
  for (const S57::ElementData& elem: elems) {
    if (elem.mode == GL_TRIANGLES) {
      int first = elem.offset / sizeof(GLuint);
      for (uint i = 0; i < elem.count / 3; i++) {
        const QPointF p0(vertices[2 * indices[first + 3 * i + 0] + 0],
                         vertices[2 * indices[first + 3 * i + 0] + 1]);
        const QPointF p1(vertices[2 * indices[first + 3 * i + 1] + 0],
                         vertices[2 * indices[first + 3 * i + 1] + 1]);
        const QPointF p2(vertices[2 * indices[first + 3 * i + 2] + 0],
                         vertices[2 * indices[first + 3 * i + 2] + 1]);

        const float da = std::abs((p1.x() - p0.x()) * (p2.y() - p0.y()) - (p2.x() - p0.x()) * (p1.y() - p0.y()));
        area += da;
        s.rx() += da / 3. * (p0.x() + p1.x() + p2.x());
        s.ry() += da / 3. * (p0.y() + p1.y() + p2.y());
      }
    } else {
      Q_ASSERT_X(false, "computeAreaCenter", "Unknown primitive");
    }
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
    qWarning() << "Cannot triangulate";
    return;
  }
  tri.addPolygon(first, count);

  for (int i = 1; i < edges.size(); i++) {
    first = edges[i].offset / sizeof(GLuint) + 1;
    count = edges[i].count - 3;
    // skip open ended linestrings
    if (indices[first] != indices[first + count]) {
      qDebug() << "TRIANGULATE: skipping";
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
      e.count += addIndices(edges[i], indices);
      prevlast = getEndPoint(i);
      i++;
    }
    // finalize current element
    const int adj = e.offset / sizeof(GLuint);
    if (prevlast == start) {
      // polygon
      indices[adj] = indices.last(); // prev
      indices.append(indices[adj + 1]); // close polygon
      indices.append(indices[adj + 2]); // adjacent = next of first
    } else {
      // line string
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


