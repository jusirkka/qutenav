#include "triangulator.h"
#include "sdfunction.h"
#include "GeoCore.hpp"
#include <algorithm>
#include <QDebug>
#include <QString>

Triangulator::Triangulator(const SD::Function* scene, scalar err)
  : m_scene(scene)
  , m_dw(5. * sqrt(err))
  , m_expandCount(-1)
  , m_splitCount(-1)
  , m_joinCount(-1) {
  m_mesh.append(vec3(0.)); // first mesh point is dummy
}

Triangulator::Triangulator(const SD::Function* scene, scalar err, int ec, int sc, int jc)
  : m_scene(scene)
  , m_dw(5. * sqrt(err))
  , m_expandCount(ec)
  , m_splitCount(sc)
  , m_joinCount(jc) {
  m_mesh.append(vec3(0.)); // first mesh point is dummy
}

Triangulator::Triangulator(const SD::Function* scene, scalar err, const Mesh& front)
  : m_scene(scene)
  , m_dw(5. * sqrt(err))
  , m_expandCount(-1)
  , m_splitCount(-1)
  , m_joinCount(-1) {
  m_mesh.append(vec3(0.)); // first mesh point is dummy
  addFront(front);
}

void Triangulator::addFront(const Mesh &points) {
  auto f = new Front(m_mesh);
  const int np = points.size();
  for (int ip = 0; ip < np; ip++) {
    // skip points that are too close
    int ipp = (ip - 1 + np) % np;
    const scalar el = glm::length(points[ip] - points[ipp]);
    if (el < SD::Function::eps) {
      // qDebug() << "skipping" << ip;
      continue;
    }
    const vec3 sp = m_scene->projectToSurface(points[ip]);
    const scalar roc = m_scene->minimalROC(sp);
    const scalar elmax = rocToEdgeLen(roc);
    // split lines that are too long
    scalar e = el - elmax;
    while (e > 0.) {
      const scalar t = 1. - e / el;
      // qDebug() << "splitting at " << t;
      const vec3 p = m_scene->projectToSurface(points[ip] * t + points[ipp] * (1 - t));
      m_mesh.append(p);
      Front::Data d;
      d.meshIndex = m_mesh.size() - 1;
      d.angleValid = false;
      d.n = m_scene->gradient(p);
      TangentSpace(d.n, d.t, d.b);
      d.roc = m_scene->minimalROC(p);
      f->append(d);
      e -= elmax;
    }

    m_mesh.append(sp);
    Front::Data sd;
    sd.meshIndex = m_mesh.size() - 1;
    sd.angleValid = false;
    sd.n = m_scene->gradient(sp);
    TangentSpace(sd.n, sd.t, sd.b);
    sd.roc = roc;

    f->append(sd);
  }

  f->finalize();
  m_fronts.push(f);
}

void Triangulator::seedAndTriangulate() {
  m_fronts.push(seedHexagon());
  triangulate();
}

void Triangulator::triangulate() {

  int ec = m_expandCount;
  int sc = m_splitCount;
  int jc = m_joinCount;

  while (!m_fronts.isEmpty()) {

    if (ec == 0 || jc == 0 || sc == 0) break;

    Front* f0 = m_fronts.pop();
    // qDebug() << "Front pop" << f0->points.size() << f0->meshIndex(0);

    try {

      while (f0->canExpand()) {

        if (ec == 0 || jc == 0 || sc == 0) break;

        scalar edgeLen;
        const scalar roc = f0->points[f0->frontIndex(0)].roc;
        if (roc > SD::Function::max_radius) {
          edgeLen = flatRegionEdgeLen(f0);
        } else {
          edgeLen = rocToEdgeLen(roc);
        }
        edgeLen = f0->edgeLen(edgeLen);

        bool splitMe;
        const quint32 collisionIndex = f0->checkIntraCollision(edgeLen, splitMe);

        if (splitMe) {
          splitFront(f0, f0->frontIndex(0), collisionIndex);
          sc -= 1;
        } else {
          bool joinUs = false;
          for (Front* f: m_fronts) {
            const quint32 joinIndex = f->checkInterCollision(f0, edgeLen, joinUs);
            if (joinUs) {
              m_fronts.removeOne(f);
              joinFronts(f0, f, f0->frontIndex(0), joinIndex);
              delete f;
              jc -= 1;
              break;
            }
          }
          if (!joinUs) {
            expand(f0, edgeLen);
            ec -= 1;
            // if (ec >= 0) qDebug() << "remaining expands" << ec;
          }
        }
      } // canExpand
      if (f0->points.size() == 3) {
        m_triangles.append(f0->meshIndex(0));
        m_triangles.append(f0->meshIndex(1));
        m_triangles.append(f0->meshIndex(-1));
        delete f0;
      } else {
        // qDebug() << "Remaining front size" << f0->points.size();
        if (ec == 0 || jc == 0 || sc == 0) {
          m_fronts.push(f0);
        }
      }
    } catch (Interrupt& e) {
      qDebug() << e.message;
      m_fronts.push(f0);
      m_interrupts.append(e.meshIndices);
      break;
    }
  }
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

  // qDebug() << "expand" << edgeLen << nt;

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
    // qDebug() << "expand roc" << d.roc;

    // FIXME/optimization: first insert nt - 1 empty items and fill them in this loop
    f->insert(f->currentIndex + i, d);
  }

  m_triangles.append(f->meshIndex(0));
  m_triangles.append(f->meshIndex(1));
  m_triangles.append(f->meshIndex(-1));
  for (quint32 i = 1; i < nt; i++) {
    m_triangles.append(f->meshIndex(0));
    m_triangles.append(f->meshIndex(i + 1));
    m_triangles.append(f->meshIndex(i));
  }

  f->deleteCurrent();
  f->finalize();

}


Triangulator::Front* Triangulator::seedHexagon() {

  const vec3 seed = m_scene->randomSurfacePoint();
  m_mesh.append(seed);
  const quint32 seedIndex = m_mesh.size() - 1;

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
  // qDebug() << "seed edge length =" << edgeLen;
  for (int i = 0; i < 6; i++) {

    const scalar x = edgeLen * cos(i * M_PI / 3);
    const scalar y = edgeLen * sin(i * M_PI / 3);

    const vec3 sp = m_scene->projectToSurface(seed + x * t + y * b);

    m_mesh.append(sp);

    Front::Data d;
    d.meshIndex = m_mesh.size() - 1;
    d.angleValid = false;
    d.n = m_scene->gradient(sp);
    TangentSpace(d.n, d.t, d.b);
    d.roc = m_scene->minimalROC(sp);
    // qDebug() << "seed roc" << d.roc;

    front->append(d);
  }

  front->finalize();

  for (int i = 0; i < 6; i++) {
    m_triangles.append(seedIndex);
    m_triangles.append(seedIndex + 1 + (i + 1) % 6);
    m_triangles.append(seedIndex + 1 + i);
  }

  return front;
}

// e.g. i1 = 1, i2 = 3, f.size = 6
// f: 1, 2, 3
// f2: 3, 4, 5, 0, 1
void Triangulator::splitFront(Front *f, quint32 i1, quint32 i2) {
  // qDebug() << "split front" << f->points.size();
  // qDebug() << f->meshIndex(i1,  -3) << f->meshIndex(i1, -2)  << f->meshIndex(i1, -1)
  //          << f->meshIndex(i1,   0) << f->meshIndex(i1,  1);
  // qDebug() << f->meshIndex(i2,  -2) << f->meshIndex(i2, -1)  << f->meshIndex(i2, 0)
  //          << f->meshIndex(i2,   1) << f->meshIndex(i2,  2);
  if (f->frontIndex(qMin(i1, i2), -qMax(i1, i2)) < 2) {
    // qDebug() << i1 << i2 << f->meshIndex(i1, 0) << f->meshIndex(i2, 0)
    //          << f->expansionAngle();
    auto msg = QString("Too small front");
    throw Interrupt(msg, Indices()
                    << f->meshIndex(i1, 0)
                    << f->meshIndex(i2, 0));
  }
  Front* f2 = new Front(m_mesh);

  quint32 nlower = qMin(i1, i2);
  quint32 nupper = f->points.size() - 1 - qMax(i1, i2);

  while (nlower > 0) {
    f2->append(f->points.first());
    f->remove(0);
    nlower -= 1;
  }
  f2->append(f->points.first());

  while (nupper > 0) {
    f2->insert(0, f->points.last());
    f->remove(f->points.size() - 1);
    nupper -= 1;
  }
  f2->insert(0, f->points.last());

  f->points.first().angleValid = false;
  f->points.last().angleValid = false;
  f->finalize();

  f2->points.first().angleValid = false;
  f2->points.last().angleValid = false;
  f2->finalize();

  // qDebug() << "current front" << f->points.size() << f->meshIndex(0);
  // qDebug() << "new front" << f2->points.size() << f2->meshIndex(0);

  m_fronts.push(f2);
}

// e.g. f1.size = 6, i1 = 3, f2.size = 4, i2 = 2
// f1: 1-0, 1-1, 1-2, 1-3, 2-2, 2-3, 2-0, 2-1, 2-2, 1-3, 1-4, 1-5
void Triangulator::joinFronts(Front *f1, const Front *f2, quint32 i1, quint32 i2) {
  // qDebug() << "join fronts" << f1->points.size() << f2->points.size();
  if (f1->points.size() == 6 && f2->points.size() == 753) {
    throw Interrupt("join", Indices()
                    << f1->meshIndex(i1, 0)
                    << f2->meshIndex(i2, 0));
  }
  Front::DataList tail1;
  quint32 nupper = f1->points.size() - i1;
  while (nupper > 0) {
    tail1.prepend(f1->points.last());
    f1->points.removeLast();
    nupper -= 1;
  }
  // f1: 0, 1, 2, tail1: 3, 4, 5

  // make copies of 1-3 and 2-2 and append to f1
  Front::Data d = tail1.first();
  d.angleValid = false;
  m_mesh.append(m_mesh[d.meshIndex]);
  d.meshIndex = m_mesh.size() - 1;
  f1->append(d);

  d = f2->points[i2];
  d.angleValid = false;
  m_mesh.append(m_mesh[d.meshIndex]);
  d.meshIndex = m_mesh.size() - 1;
  f1->append(d);

  // f1: 1-0, 1-1, 1-2, copy-of-1-3, copy-of-2-2
  for (int i = 0; i < f2->points.size(); i++) {
    f1->append(f2->points[(i2 + 1 + i) % f2->points.size()]);
  }
  // f1: 1-0, 1-1, 1-2, copy-of-1-3, copy-of-2-2, 2-3, 2-0, 2-1, 2-2

  f1->points.last().angleValid = false;
  tail1.first().angleValid = false;

  f1->points.append(tail1);

  // f1: 1-0, 1-1, 1-2, copy-of-1-3, copy-of-2-2, 2-3, 2-0, 2-1, 2-2, 1-3, 1-4, 1-5
  f1->finalize();
  // qDebug() << "joined size" << f1->points.size();
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
  // qDebug() << "flatRegionEdgeLen:" << edgeLen;
  return edgeLen;
}

scalar Triangulator::rocToEdgeLen(scalar roc) const {
  return m_dw * roc;
}


void Triangulator::write(QFile& f) {
  QTextStream out(&f);

  // remove the dummy item
  m_mesh.removeFirst();

  out << "# Vertices\n";
  for (const vec3& v: m_mesh) {
    out << "v " << v.x << " " << v.y << " " << v.z << "\n";
  }
  //  out << "\n# Normals\n";
  //  for (const vec3& v: m_mesh) {
  //    const vec3 n = m_scene->gradient(v);
  //    out << "vn " << n.x << " " << n.y << " " << n.z << "\n";
  //  }
  out << "\n# Faces";
  int cnt = 0;
  for (quint32 index: m_triangles) {
    if (cnt % 3 == 0) {
      out << "\nf " << index << "//";
    } else {
      out << " " << index << "//";
    }
    cnt += 1;
  }
  out << "\n# Fronts";


  for (const Front* f: m_fronts) {
    cnt = 1;
    out << "\nl";
    for (const Front::Data& d: f->points) {
      if (cnt % 10 == 0) {
        out << " \\\n  " << d.meshIndex;
      } else {
        out << " " << d.meshIndex;
      }
      cnt += 1;
    }
    out << " " << f->points.first().meshIndex;
  }

  // push back the dummy item
  m_mesh.prepend(vec3(0.));

  out << "\n# Interrupts\n";

  for (quint32 mi: m_interrupts) {
    writeInterrupt(out, mi);
  }


}

void Triangulator::writeInterrupt(QTextStream &out, quint32 mi) {
  const vec3 v = m_mesh[mi];
  const vec3 n = m_scene->gradient(v);
  vec3 t, b;
  TangentSpace(n, t, b);
  const scalar s = .03 * rocToEdgeLen(m_scene->minimalROC(v));
  vec3 p = m_scene->projectToSurface(v + s * t + s * b);
  out << "v " << p.x << " " << p.y << " " << p.z << "\n";
  p = m_scene->projectToSurface(v - s * t + s * b);
  out << "v " << p.x << " " << p.y << " " << p.z << "\n";
  p = m_scene->projectToSurface(v - s * t - s * b);
  out << "v " << p.x << " " << p.y << " " << p.z << "\n";
  p = m_scene->projectToSurface(v + s * t - s * b);
  out << "v " << p.x << " " << p.y << " " << p.z << "\n";

  out << "l -1 -4 -3 -2 -1\n";
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

quint32 Triangulator::Front::distance(quint32 fi) const {
  const quint32 i1 = (currentIndex - fi + points.size()) % points.size();
  const quint32 i2 = (fi - currentIndex + points.size()) % points.size();
  return qMin(i1, i2);
}


scalar Triangulator::Front::expansionAngle() const {
  return points[currentIndex].openingAngle;
}



quint32 Triangulator::Front::checkIntraCollision(scalar radius, bool &found) const {
  const vec3& p = adaptor.mesh[meshIndex(0)];
  const scalar query_pt[3] = {p.x, p.y, p.z};
  found = false;
  const scalar r2max = radius * radius;
  const scalar eps2 = SD::Function::eps * SD::Function::eps;

  const QVector<quint32> excludes{
    meshIndex(-1),
    meshIndex(0),
    meshIndex(1),
  };

  scalar r2 = 0.;
  size_t num_results = 10;
  size_t k0 = 0;
  while (r2 <= r2max) {
    size_t ret_index[num_results];
    scalar dist_sqr[num_results];
    nanoflann::KNNResultSet<scalar> resultSet(num_results);
    resultSet.init(ret_index, dist_sqr);
    db.findNeighbors(resultSet, query_pt, nanoflann::SearchParams(10));
    for (size_t k = k0; k < resultSet.size(); k++) {
      r2 = dist_sqr[k];
      // after join there are common points
      if (r2 < eps2) continue;
      if (r2 > r2max) return 0;
      const quint32 mi = adaptor.indices[ret_index[k]];
      if (excludes.contains(mi)) continue;
      if (adaptor.mfLookup.contains(mi)) {
        // check for bad near points
        const scalar oa = computeOpeningAngle(frontIndex(0), mi);
        if (oa < points[frontIndex(0)].openingAngle) {
          return checkIntrasect(adaptor.mfLookup[mi], found);
        }
      }
    } // loop result set
    // no more points?
    if (resultSet.size() < num_results) return 0;
    // not found but there are still point within radius
    k0 = resultSet.size();
    num_results += 10;
  }
  return 0;
}


quint32 Triangulator::Front::checkInterCollision(const Front* current, scalar radius, bool &found) const {
  const vec3& p = current->adaptor.mesh[current->meshIndex(0)];
  const scalar query_pt[3] = {p.x, p.y, p.z};
  found = false;
  const scalar r2max = radius * radius;
  const scalar eps2 = SD::Function::eps * SD::Function::eps;


  scalar r2 = 0.;
  size_t num_results = 10;
  size_t k0 = 0;
  while (r2 <= r2max) {
    size_t ret_index[num_results];
    scalar dist_sqr[num_results];
    nanoflann::KNNResultSet<scalar> resultSet(num_results);
    resultSet.init(ret_index, dist_sqr);
    db.findNeighbors(resultSet, query_pt, nanoflann::SearchParams(10));
    for (size_t k = k0; k < resultSet.size(); k++) {
      r2 = dist_sqr[k];
      // after split there are common points
      if (r2 < eps2) continue;
      if (r2 > r2max) return 0;
      const quint32 mi = adaptor.indices[ret_index[k]];
      if (adaptor.mfLookup.contains(mi)) {
        // check for bad near points
        const scalar oa = current->computeOpeningAngle(current->frontIndex(0), mi);
        if (oa < current->points[current->frontIndex(0)].openingAngle) {
          found = current->checkIntersect(mi);
          return adaptor.mfLookup[mi];
        }
      }
    } // loop result set
    // no more points?
    if (resultSet.size() < num_results) return 0;
    // not found but there are still points within radius
    k0 = resultSet.size();
    num_results += 10;
  }
  return 0;
}

quint32 Triangulator::Front::checkIntrasect(quint32 collisionIndex, bool& ok) const {

  ok = false;

  const vec3& t = points[frontIndex(0)].t;
  const vec3& b = points[frontIndex(0)].b;

  const quint32 mi = points[collisionIndex].meshIndex;

  const vec3 p0 = adaptor.mesh[mi] - adaptor.mesh[meshIndex(0)];
  const vec2 p = vec2(glm::dot(t, p0), glm::dot(b, p0));


  const int n = 5;
  if (points.size() <= 2 * n + 2) {
    // qDebug() << "small front";
  }
  QMap<quint32, quint32> edges;
  if (points.size() <= 2 * n + 2) {
    for (int i = 1; frontIndex(i) != frontIndex(-1); i++) {
      edges[frontIndex(i)] = frontIndex(i + 1);
    }
  } else {
    for (int i = 0; i < n; i++) {
      edges[frontIndex(i + 1)] = frontIndex(i + 2);
      edges[frontIndex(-i - 1)] = frontIndex(-i - 2);
    }
  }

  QMapIterator<quint32, quint32> it(edges);
  while (it.hasNext()) {
    it.next();
    const quint32 i1 = it.key();
    const quint32 i2 = it.value();
    const quint32 m1 = points[i1].meshIndex;
    const quint32 m2 = points[i2].meshIndex;

    const vec3& q01 = adaptor.mesh[m1] - adaptor.mesh[meshIndex(0)];
    const vec2 q1 = vec2(glm::dot(t, q01), glm::dot(b, q01));
    const vec3& q02 = adaptor.mesh[m2] - adaptor.mesh[meshIndex(0)];
    const vec2 q2 = vec2(glm::dot(t, q02), glm::dot(b, q02));
    if (intersect(p, q1, q2)) {
      // qDebug() << "intrasect" << m1 << m2 << points[frontIndex(0)].openingAngle;
      const scalar oa1 = computeOpeningAngle(frontIndex(0), m1);
      const scalar oa2 = computeOpeningAngle(frontIndex(0), m2);
      quint32 im, ip;
      scalar oap, oam;
      if (oa1 < oa2) {
        im = i1;
        ip = i2;
        oam = oa1;
        oap = oa2;
      } else {
        im = i2;
        ip = i1;
        oam = oa2;
        oap = oa1;
      }
      if (distance(im) > 1 && oam < points[frontIndex(0)].openingAngle) {
        ok = true;
        return im;
      }
      if (distance(ip) > 1 && oap < points[frontIndex(0)].openingAngle) {
        ok = true;
        return ip;
      }
      // not ok to split - try expanding & pray all is well
      return collisionIndex;
    }
  }
  // no intersects
  ok = true;
  return collisionIndex;
}


bool Triangulator::Front::checkIntersect(quint32 mi) const {

  const vec3& t = points[frontIndex(0)].t;
  const vec3& b = points[frontIndex(0)].b;

  const vec3 p0 = adaptor.mesh[mi] - adaptor.mesh[meshIndex(0)];
  const vec2 p = vec2(glm::dot(t, p0), glm::dot(b, p0));

  const int n = 5;
  if (points.size() <= 2 * n + 2) {
    // qDebug() << "small front";
  }
  QMap<quint32, quint32> edges;
  if (points.size() <= 2 * n + 2) {
    for (int i = 1; frontIndex(i) != frontIndex(-1); i++) {
      edges[frontIndex(i)] = frontIndex(i + 1);
    }
  } else {
    for (int i = 0; i < n; i++) {
      edges[frontIndex(i + 1)] = frontIndex(i + 2);
      edges[frontIndex(-i - 1)] = frontIndex(-i - 2);
    }
  }

  QMapIterator<quint32, quint32> it(edges);
  while (it.hasNext()) {
    it.next();
    const quint32 i1 = it.key();
    const quint32 i2 = it.value();
    const quint32 m1 = points[i1].meshIndex;
    const quint32 m2 = points[i2].meshIndex;

    const vec3& q01 = adaptor.mesh[m1] - adaptor.mesh[meshIndex(0)];
    const vec2 q1 = vec2(glm::dot(t, q01), glm::dot(b, q01));
    const vec3& q02 = adaptor.mesh[m2] - adaptor.mesh[meshIndex(0)];
    const vec2 q2 = vec2(glm::dot(t, q02), glm::dot(b, q02));
    if (intersect(p, q1, q2)) {
      // not ok to join
      return false;
    }
  }
  // no intersects - ok to join
  return true;
}


static scalar perp(const vec2& u, const vec2& v) {
  return u.x * v.y - u.y * v.x;
}

// http://geomalgorithms.com/a05-_intersect-1.html
bool Triangulator::intersect(const vec2 &p, const vec2 &q0, const vec2 &q1) {
  // assumptions: 1) p != 0, 2) q1 != q0, 3) q1 - q0 and p are not collinear
  const vec2 v = q1 - q0;
  const vec2 w = -q0;
  const scalar D = perp(p, v);

  // test if  they are parallel
  if (std::abs(D) < SD::Function::eps) {
    return false;
  }

  // the segments are skew
  // get the intersect parameter for p
  const scalar s = perp(v, w) / D;
  if (s < SD::Function::eps || s > 1. - SD::Function::eps) return false;

  // get the intersect parameter for q1 - q0
  const scalar t = perp(p, w) / D;
  if (t < SD::Function::eps || t > 1. - SD::Function::eps) return false;

  return true;
}


void Triangulator::Front::remove(quint32 i) {
  const quint32 mi = points[i].meshIndex;
  points.removeAt(i);
  if (adaptor.maLookup.contains(mi)) {
    const quint32 ai = adaptor.maLookup[mi];
    adaptor.indices[ai] = 0; // dummy index
    db.removePoint(ai);
    adaptor.maLookup.remove(mi);
  }
}


void Triangulator::Front::insert(quint32 i, const Data& d) {
  points.insert(i, d);
  adaptor.indices.append(d.meshIndex);
  adaptor.maLookup[d.meshIndex] = adaptor.indices.size() - 1;
  adaptor.newPoints += 1;
}

void Triangulator::Front::append(const Data& d) {
  points.append(d);
  adaptor.indices.append(d.meshIndex);
  adaptor.maLookup[d.meshIndex] = adaptor.indices.size() - 1;
  adaptor.newPoints += 1;
}

void Triangulator::Front::deleteCurrent() {
  remove(currentIndex);
  currentIndex = 0;
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

scalar Triangulator::Front::edgeLen(scalar baseLen) const {
  const vec3& v0 = adaptor.mesh[meshIndex(0)];
  const vec3& v1 = adaptor.mesh[meshIndex(-1)];
  const vec3& v2 = adaptor.mesh[meshIndex(1)];

  const scalar l1 = glm::length(v1 - v0);
  const scalar l2 = glm::length(v2 - v0);

  return qMin(baseLen, 2 * qMax(l1, l2));
}

void Triangulator::Front::finalize() {
  scalar minAngle = 3 * M_PI; // = infinity
  adaptor.mfLookup.clear();
  for (int i = 0; i < points.size(); i++) {
    if (!points[i].angleValid) {
      points[i].openingAngle = computeOpeningAngle(i, meshIndex(i, 1));
      points[i].angleValid = true;
    }
    if (minAngle > points[i].openingAngle) {
      currentIndex = i;
      minAngle = points[i].openingAngle;
    }
    adaptor.mfLookup[points[i].meshIndex] = i;
  }

  if (adaptor.newPoints > 0) {
    const quint32 last = adaptor.indices.size() - 1;
    db.addPoints(last - adaptor.newPoints + 1, last);
    adaptor.newPoints = 0;
  }
}

