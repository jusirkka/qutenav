#include "triangulator.h"
#include "sdfunction.h"
#include "GeoCore.hpp"
#include <QTextStream>
#include <algorithm>
#include <QDebug>

Triangulator::Triangulator(const SD::Function* scene, scalar err, int ec)
  : m_scene(scene)
  , m_err(err)
  , m_expandCount(ec) {
  m_fronts.push(seedHexagon());
}


void Triangulator::triangulate() {

  int ec = m_expandCount;

  while (!m_fronts.isEmpty()) {

    qDebug() << "Front pop";
    Front* f0 = m_fronts.pop();

    while (f0->canExpand()) {

      scalar edgeLen;
      const scalar roc = f0->points[f0->frontIndex(0)].roc;
      if (roc > SD::Function::max_radius) {
        edgeLen = flatRegionEdgeLen(f0);
      } else {
        edgeLen = rocToEdgeLen(roc);
      }

      bool splitMe;
      const quint32 collisionIndex = f0->checkIntraCollision(edgeLen, splitMe);

      if (splitMe) {
        splitFront(f0, f0->frontIndex(0), collisionIndex);
      } else {
        bool joinUs = false;
        for (Front* f: m_fronts) {
          const quint32 joinIndex = f->checkInterCollision(f0, edgeLen, joinUs);
          if (joinUs) {
            m_fronts.removeOne(f);
            joinFronts(f0, f, f0->frontIndex(0), joinIndex);
            delete f;
            break;
          }
        }
        if (!joinUs) {
          expand(f0, edgeLen);
          ec -= 1;
          if (ec > 0) qDebug() << "remaining expands" << ec;
          if (ec == 0) break;
        }
      }
    }
    m_triangles.append(f0->meshIndex(0));
    m_triangles.append(f0->meshIndex(1));
    m_triangles.append(f0->meshIndex(-1));
    delete f0;
  }
}

void Triangulator::expand(Front *f) {
  qDebug() << "expand";
  scalar edgeLen;
  const scalar roc = f->points[f->frontIndex(0)].roc;
  if (roc > SD::Function::max_radius) {
    edgeLen = flatRegionEdgeLen(f);
  } else {
    edgeLen = rocToEdgeLen(roc);
  }
  expand(f, edgeLen);
}


void Triangulator::expand(Front *f, scalar edgeLen) {
  const scalar w = f->expansionAngle();
  quint32 nt = quint32(3 * w / M_PI) + 1;
  scalar dw =  w / nt;
  const vec3& v0 = m_mesh[f->meshIndex(0)];
  const vec3& v1 = m_mesh[f->meshIndex(-1)];
  const vec3& v2 = m_mesh[f->meshIndex(1)];

  if (nt > 1 && dw < .8) {
    nt -= 1;
    dw = w / nt;
  }
  if (nt == 1 && dw > .8 && glm::length(v1 - v2) > 1.2 * edgeLen) {
    nt = 2;
    dw /= 2;
  }
  if (w < 3 && (glm::length(v1 - v0) < .5 * edgeLen || glm::length(v2 - v0) < .5 * edgeLen)) {
    nt = 1;
  }

  qDebug() << "expand" << edgeLen << nt;

  // these need to be recalculated
  f->points[f->frontIndex(-1)].angleValid = false;
  f->points[f->frontIndex(1)].angleValid = false;

  const vec3& t = f->points[f->frontIndex(0)].t;
  const vec3& b = f->points[f->frontIndex(0)].b;
  const vec2 x0 = glm::normalize(vec2(glm::dot(t, v1 - v0), glm::dot(b, v1 - v0)));
  for (quint32 i = 1; i < nt; i++) {

    const scalar s = sin(i * dw);
    const scalar c = cos(i * dw);

    const scalar x = edgeLen * (c * x0.x - s * x0.y);
    const scalar y = edgeLen * (s * x0.x + c * x0.y);
    const vec3 sp = m_scene->projectToSurface(v0 + x * t + y * b);

    m_mesh.append(sp);

    Front::Data d;
    d.meshIndex = m_mesh.size() - 1;
    d.angleValid = false;
    d.n = m_scene->gradient(sp);
    TangentSpace(d.n, d.t, d.b);
    d.roc = m_scene->minimalROC(sp);
    qDebug() << "expand roc" << d.roc;

    // FIXME/optimization: first insert nt - 1 empty items and fill them in this loop
    qDebug() << "new point" << f->frontIndex(i) << d.meshIndex;
    f->points.insert(f->currentIndex + i, d);
    f->adaptor.indices.append(d.meshIndex);
  }

  if (nt > 1) {
    const quint32 last = f->adaptor.indices.size() - 1;
    f->db.addPoints(last - nt + 2, last);
  }

//  qDebug() << "0th triangle" << f->meshIndex(0) << f->meshIndex(1) << f->meshIndex(-1);
//  qDebug() << "0th triangle" << f->frontIndex(0) << f->frontIndex(1) << f->frontIndex(-1);
  m_triangles.append(f->meshIndex(0));
  m_triangles.append(f->meshIndex(1));
  m_triangles.append(f->meshIndex(-1));
  for (quint32 i = 1; i < nt; i++) {
//    qDebug() << i << "th triangle" << f->meshIndex(0) << f->meshIndex(i+1) << f->meshIndex(i);
//    qDebug() << i << "th triangle" << f->frontIndex(0) << f->frontIndex(i+1) << f->frontIndex(i);
    m_triangles.append(f->meshIndex(0));
    m_triangles.append(f->meshIndex(i + 1));
    m_triangles.append(f->meshIndex(i));
  }

  f->deleteCurrent();
  f->computeAngles();

}


Triangulator::Front* Triangulator::seedHexagon() {

  // first index = dummy index
  m_mesh.append(vec3(0.));

  const vec3 seed = m_scene->randomSurfacePoint();
  m_mesh.append(seed);

  auto front = new Front(m_mesh);

  const vec3 n = m_scene->gradient(seed);
  vec3 t, b;
  TangentSpace(n, t, b);

  scalar edgeLen;
  const scalar roc = m_scene->minimalROC(seed);
  if (roc > SD::Function::max_radius) {
    edgeLen = flatRegionEdgeLen(seed, t, b, 2 * M_PI);
  } else {
    edgeLen = rocToEdgeLen(roc);
  }
  qDebug() << "seed edge length =" << edgeLen;
  for (int i = 0; i < 6; i++) {

    const scalar x = edgeLen * cos(i * M_PI / 3);
    const scalar y = edgeLen * sin(i * M_PI / 3);

    const vec3 sp = m_scene->projectToSurface(seed + x * t + y * b);

    m_mesh.append(sp);

    Front::Data d;
    d.meshIndex = i + 2;
    d.angleValid = false;
    d.n = m_scene->gradient(sp);
    TangentSpace(d.n, d.t, d.b);
    d.roc = m_scene->minimalROC(sp);
    qDebug() << "seed roc" << d.roc;

    front->points.append(d);
    front->adaptor.indices.append(d.meshIndex);
  }
  front->db.addPoints(0, 5);

  front->computeAngles();

  for (int i = 0; i < 6; i++) {
    m_triangles.append(1);
    m_triangles.append((i + 1) % 6 + 2);
    m_triangles.append(i + 2);
  }

  return front;
}

// e.g. i1 = 1, i2 = 3, f.size = 6
// f: 1, 2, 3
// f2: 3, 4, 5, 0, 1
void Triangulator::splitFront(Front *f, quint32 i1, quint32 i2) {
  qDebug() << "split front" << f->points.size();
  Front* f2 = new Front(m_mesh);

  quint32 nlower = qMin(i1, i2);
  quint32 nupper = f->points.size() - 1 - qMax(i1, i2);

  while (nlower > 0) {
    f2->points.append(f->points.first());
    f->removeFromTree(f->points.first().meshIndex);
    f->points.removeFirst();
    nlower -= 1;
  }
  f2->points.append(f->points.first());

  while (nupper > 0) {
    f2->points.prepend(f->points.last());
    f->removeFromTree(f->points.last().meshIndex);
    f->points.removeLast();
    nupper -= 1;
  }
  f2->points.prepend(f->points.last());

  f->points.first().angleValid = false;
  f->points.last().angleValid = false;
  qDebug() << "current front computeAngles";
  f->computeAngles();

  f2->points.first().angleValid = false;
  f2->points.last().angleValid = false;
  qDebug() << "new front computeAngles";
  f2->computeAngles();

  qDebug() << "current front" << f->points.size();
  qDebug() << "new front" << f2->points.size();
  f2->addToTree(f2->points);
  m_fronts.push(f2);
}

// e.g. f1.size = 6, i1 = 3, f2.size = 4, i2 = 2
// f1: 1-0, 1-1, 1-2, 1-3, 2-2, 2-3, 2-0, 2-1, 1-3, 1-4, 1-5
void Triangulator::joinFronts(Front *f1, const Front *f2, quint32 i1, quint32 i2) {
  qDebug() << "join" << i1 << i2;
  Front::DataList tail1;
  quint32 nupper = f1->points.size() - 1 - i1;
  while (nupper > 0) {
    tail1.prepend(f1->points.last());
    f1->points.removeLast();
    nupper -= 1;
  }
  tail1.append(f1->points.last());
  // f1: 0, 1, 2, 3, tail1: 3, 4, 5
  f1->points.append(f2->points[i2]);
  f1->points.append(f2->points[(i2 + 1) % f2->points.size()]);
  // f1: 1-0, 1-1, 1-2, 1-3, 2-2, 2-3
  const quint32 k1 = f1->points.size() - 3;
  const quint32 k2 = f1->points.size() - 2;
  const quint32 m1 = f1->points[k1].meshIndex;

  f1->points[k1].angleValid = false;
  f1->points[k2].angleValid = false;
  f1->computeAngles();

  const scalar a1 = f1->points[k1].openingAngle;
  const scalar a2 = f1->points[k2].openingAngle;

  if (a1 < a2) {
    f1->currentIndex = k1;
    expand(f1);
    // new front points inserted after current index
    f1->currentIndex = f1->points.size() - 2;
    expand(f1);
  } else {
    f1->currentIndex = k2;
    expand(f1);
    // new front points inserted after current index: safe to use i1 < i2
    f1->currentIndex = k1;
    expand(f1);
  }
  // f1: 1-0, 1-1, 1-2, <new points>, 2-3
  for (int i = 0; i < f2->points.size() - 1; i++) {
    f1->points.append(f2->points[(i2 + 2 + i) % f2->points.size()]);
  }
  // f1: 1-0, 1-1, 1-2, <new points>, 2-3, 2-0, 2-1, 2-2
  f1->points.last().angleValid = false;
  tail1.first().angleValid = false;
  f1->points.append(tail1);
  // f1: 1-0, 1-1, 1-2, <new points>, 2-3, 2-0, 2-1, 2-2, 1-3, 1-4, 1-5
  f1->computeAngles();
  f1->addToTree(f2->points);
  // point 1-3 removed from tree when expanding so add it back
  f1->addToTree(m1);
}


scalar Triangulator::flatRegionEdgeLen(const Front *f) const {
  const vec3& v0 = m_mesh[f->meshIndex(0)];
  const vec3& v1 = m_mesh[f->meshIndex(-1)];
  const vec3& t = f->points[f->frontIndex(0)].t;
  const vec3& b = f->points[f->frontIndex(0)].b;
  const vec2 x0 = glm::normalize(vec2(glm::dot(t, v1 - v0), glm::dot(b, v1 - v0)));
  const scalar oa = f->points[f->frontIndex(0)].openingAngle;
  return flatRegionEdgeLen(v0, x0.x * t + x0.y * b, -x0.y * t + x0.x * b, oa);
}

scalar Triangulator::flatRegionEdgeLen(const vec3 &p,
                                       const vec3 &x, const vec3 &y,
                                       scalar oa) const {
  // probe the region to find the flat region boundaries
  scalar edgeLen = SD::Function::max_radius;
  const scalar eps = 1.e-4;
  scalar w = qMin(M_PI / 18, oa / 2);
  do {
    const scalar s = sin(w);
    const scalar c = cos(w);
    scalar len = 0.;
    scalar d;
    do {
      len += 1.;
      const vec3 p1 = p + len * (c * x + s * y);
      d = std::abs(m_scene->signedDistance(p1));
      // qDebug() << "flatRegionEdgeLen:" << len << d << x.x << x.y << x.z;
    } while (d < eps);
    len -= d;
    if (len < edgeLen) edgeLen = len;
    w += M_PI / 18;
  } while (w < oa);
   qDebug() << "flatRegionEdgeLen:" << edgeLen;
  return edgeLen;
}

scalar Triangulator::rocToEdgeLen(scalar roc) const {
  return 5. * sqrt(m_err) * roc;
}


void Triangulator::write(QFile& f) {
  QTextStream out(&f);

  // remove the dummy item
  m_mesh.removeFirst();

  out << "# Vertices\n";
  for (const vec3& v: m_mesh) {
    out << "v " << v.x << " " << v.y << " " << v.z << "\n";
  }
  out << "\n# Normals\n";
  for (const vec3& v: m_mesh) {
    const vec3 n = m_scene->gradient(v);
    out << "vn " << n.x << " " << n.y << " " << n.z << "\n";
  }
  out << "\n# Faces";
  int cnt = 0;
  for (quint32 index: m_triangles) {
    // vertices and normals are in 1-to-1 order
    if (cnt % 3 == 0) {
      out << "\nf " << index << "//" << index;
    } else {
      out << " " << index << "//" << index;
    }
    cnt += 1;
  }
  out << "\n";

  // push back the dummy item
  m_mesh.prepend(vec3(0.));

}

//
// Front implementation
//

Triangulator::Front::Front(const Mesh &mesh)
  : adaptor(mesh)
  , db(3, adaptor, nanoflann::KDTreeSingleIndexAdaptorParams(10))
{}

bool Triangulator::Front::canExpand() const {
  return points.size() > 3;
}

quint32 Triangulator::Front::meshIndex(qint32 diff) const {
  return points[(currentIndex + points.size() + diff) % points.size()].meshIndex;
}

quint32 Triangulator::Front::meshIndex(quint32 base, qint32 diff) const {
  return points[(base + points.size() + diff) % points.size()].meshIndex;
}

quint32 Triangulator::Front::frontIndex(qint32 diff) const {
  return (currentIndex + points.size() + diff) % points.size();
}

quint32 Triangulator::Front::frontIndex(quint32 base, qint32 diff) const {
  return (base + points.size() + diff) % points.size();
}

scalar Triangulator::Front::expansionAngle() const {
  return points[currentIndex].openingAngle;
}

void Triangulator::Front::deleteCurrent() {
  removeFromTree(points[currentIndex].meshIndex);
  points.removeAt(currentIndex);
  currentIndex = 0;
}

void Triangulator::Front::removeFromTree(quint32 mi) {
  const int i = adaptor.indices.indexOf(mi);
  if (i < 0) return;
  adaptor.indices[i] = 0; // dummy index
  db.removePoint(i);
}

void Triangulator::Front::addToTree(const DataList& dl) {
  for (const Data& d: dl) {
    adaptor.indices.append(d.meshIndex);
  }
  const int last = adaptor.indices.size() - 1;
  db.addPoints(last - dl.size() + 1, last);
}

void Triangulator::Front::addToTree(quint32 mi) {
  const int i = adaptor.indices.indexOf(mi);
  if (i >= 0) return;
  adaptor.indices.append(mi);
  const int last = adaptor.indices.size() - 1;
  db.addPoints(last, last);
}

quint32 Triangulator::Front::checkInterCollision(const Front* target, scalar radius, bool &found) const {
  const size_t num_results = 1;
  size_t ret_index;
  scalar dist_sqr;
  nanoflann::KNNResultSet<scalar> resultSet(num_results);
  resultSet.init(&ret_index, &dist_sqr);
  const vec3& p = target->adaptor.mesh[target->meshIndex(0)];
  const scalar query_pt[3] = {p.x, p.y, p.z};
  qDebug() << "checkInterCollision" << adaptor.indices.size() << points.size();
  db.findNeighbors(resultSet, query_pt, nanoflann::SearchParams(10));
  found = false;

  if (resultSet.size() < num_results) {
    return 0;
  }
  // fronts have common points after split
  QVector<quint32> excludes{
    target->meshIndex(-2),
    target->meshIndex(-1),
    target->meshIndex(0),
    target->meshIndex(1),
    target->meshIndex(2)
  };
  if (excludes.contains(adaptor.indices[ret_index])) {
    return 0;
  }
  if (dist_sqr < radius * radius) {
    qDebug() << "checkInterCollision" << sqrt(dist_sqr) << radius
             << adaptor.indices.size() << points.size();
    const quint32 mi = adaptor.indices[ret_index];
    for (int i = 0; i < points.size(); i++) {
      if (points[i].meshIndex == mi) {
        // check for bad near points
        const scalar oa = target->computeOpeningAngle(target->frontIndex(0), points[i].meshIndex);
        if (oa < target->points[target->frontIndex(0)].openingAngle) {
          qDebug() << oa * 180 / M_PI << target->points[target->frontIndex(0)].openingAngle * 180 / M_PI;
          found = true;
        }
        return i;
      }
    }
  }

  return 0;
}

quint32 Triangulator::Front::checkIntraCollision(scalar radius, bool &found) const {
  const size_t num_results = 6;
  size_t ret_index[num_results];
  scalar dist_sqr[num_results];
  nanoflann::KNNResultSet<scalar> resultSet(num_results);
  resultSet.init(ret_index, dist_sqr);
  const vec3& p = adaptor.mesh[meshIndex(0)];
  const scalar query_pt[3] = {p.x, p.y, p.z};
  qDebug() << "checkIntraCollision" << adaptor.indices.size() << points.size();
  db.findNeighbors(resultSet, query_pt, nanoflann::SearchParams(10));
  found = false;

  QVector<quint32> excludes{
    meshIndex(-2),
    meshIndex(-1),
    meshIndex(0),
    meshIndex(1),
    meshIndex(2)
  };

  for (size_t k = 0; k < resultSet.size(); k++) {
    if (excludes.contains(adaptor.indices[ret_index[k]])) continue;
    qDebug() << "checkIntraCollision" << k
             << sqrt(dist_sqr[k])
             << radius
             << adaptor.indices.size()
             << points.size();
    if (dist_sqr[k] < radius * radius) {
      for (int i = 0; i < points.size(); i++) {
        if (points[i].meshIndex == adaptor.indices[ret_index[k]]) {
          // check for bad near points
          const scalar oa = computeOpeningAngle(frontIndex(0), points[i].meshIndex);
          if (oa < points[frontIndex(0)].openingAngle) {
            qDebug() << oa * 180 / M_PI << points[frontIndex(0)].openingAngle * 180 / M_PI;
            found = true;
          }
          return i;
        }
      }
    }
  }
  return 0;
}

scalar Triangulator::Front::computeOpeningAngle(quint32 fi, quint32 mi) const {
  const vec3& v0 = adaptor.mesh[meshIndex(fi, 0)];
  const vec3& v1 = adaptor.mesh[meshIndex(fi, -1)];
  const vec3& v2 = adaptor.mesh[mi];

  const vec3& t = points[fi].t;
  const vec3& b = points[fi].b;

  scalar w1 = atan2(glm::dot(b, v1 - v0), glm::dot(t, v1 - v0));
  scalar w2 = atan2(glm::dot(b, v2 - v0), glm::dot(t, v2 - v0));

  if (w1 < 0.) w1 += 2 * M_PI;
  if (w2 < 0.) w2 += 2 * M_PI;

  if (w2 > w1) {
    return w2 - w1;
  }

  return w2 - w1 + 2 * M_PI;
}

void Triangulator::Front::computeAngles() {
  scalar minAngle = 3 * M_PI; // = infinity
  for (int i = 0; i < points.size(); i++) {
    if (!points[i].angleValid) {
      points[i].openingAngle = computeOpeningAngle(i, meshIndex(i, 1));
      points[i].angleValid = true;
    }
    if (minAngle > points[i].openingAngle) {
      currentIndex = i;
      minAngle = points[i].openingAngle;
    }
  }
}

