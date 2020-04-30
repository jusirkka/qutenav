#pragma once

#include <QStack>
#include <QFile>
#include <QTextStream>
#include <QList>
#include "nanoflann.hpp"
#include <QMap>

#include "precision.h"

namespace SD {
class Function;
}

using Indices = QVector<quint32>;

class Interrupt {
public:
  Interrupt(QString msg, Indices mi)
    : message(std::move(msg))
    , meshIndices(std::move(mi)) {}
  const QString message;
  const Indices meshIndices;
};

class Triangulator {
public:
  using Mesh = QVector<vec3>;
  Triangulator(const SD::Function* scene, scalar err, int ec, int sc, int jc);
  Triangulator(const SD::Function* scene, scalar err, const Mesh& front);
  Triangulator(const SD::Function* scene, scalar err);
  void triangulate();
  void seedAndTriangulate();
  void addFront(const Mesh& front);
  void write(QFile& f);

private:


  struct Front {

    Front(const Mesh& mesh);

    struct Data {
      quint32 meshIndex;
      scalar openingAngle; // in radians
      scalar roc;
      bool angleValid;
      vec3 n;
      vec3 t;
      vec3 b;
    };

    using DataList = QVector<Data>;

    struct Adaptor {
      Adaptor(const Mesh& m)
        : mesh(m)
        , newPoints(0) {}

      inline unsigned int kdtree_get_point_count() const {
        return indices.size();
      }
      inline float kdtree_get_pt(const unsigned int i, int dim) const {
        return mesh[indices[i]][dim];
      }
      template <class BBOX> bool kdtree_get_bbox(BBOX&) const {
        return false;
      }

      using IndexLookup = QMap<quint32, quint32>;
      using PointerLookup = QMap<quint32, Data*>;
      const Mesh& mesh;
      Indices indices;
      IndexLookup maLookup; // key = mesh index, value adaptor index
      IndexLookup mfLookup; // key = mesh index, value front index
      quint32 newPoints;
    };

    using Front_pt_kdtree = nanoflann::KDTreeSingleIndexDynamicAdaptor
    <nanoflann::L2_Simple_Adaptor<scalar, Adaptor>, Adaptor, 3>;


    bool canExpand() const;
    quint32 meshIndex(qint32 diff) const;
    quint32 meshIndex(quint32 base, qint32 diff) const;
    quint32 frontIndex(qint32 diff) const;
    quint32 frontIndex(quint32 base, qint32 diff) const;
    quint32 distance(quint32 fi) const;
    scalar expansionAngle() const;
    scalar computeOpeningAngle(quint32 fi, quint32 mi) const;
    scalar edgeLen(scalar baseLen) const;
    quint32 checkInterCollision(const Front* target, scalar radius, bool& found) const;
    quint32 checkIntraCollision(scalar radius, bool& found) const;
    bool checkIntersect(quint32 meshIndex) const;
    quint32 checkIntrasect(quint32 collisionIndex, bool& found) const;

    void remove(quint32 i);
    void insert(quint32 i, const Data& d);
    void append(const Data& d);
    void deleteCurrent();
    void finalize();

    quint32 currentIndex;
    DataList points;
    Adaptor adaptor;
    Front_pt_kdtree db;
  };

  using FrontStack = QStack<Front*>;

  Front* seedHexagon();
  void expand(Front* f, scalar edgeLen);

  void splitFront(Front* f, quint32 i1, quint32 i2);
  void joinFronts(Front* f1, const Front* f2, quint32 i1, quint32 i2);

  static bool intersect(const vec2& p, const vec2& q0, const vec2& q1);

  scalar rocToEdgeLen(scalar roc) const;
  scalar flatRegionEdgeLen(const Front* f) const;
  scalar flatRegionEdgeLen(const vec3& p, const vec3& x, const vec3& y, scalar oa) const;

  void writeInterrupt(QTextStream& f, quint32 mi);

  const SD::Function* m_scene;
  FrontStack m_fronts;
  Mesh m_mesh;
  Indices m_triangles;
  scalar m_dw;
  int m_expandCount;
  int m_splitCount;
  int m_joinCount;
  Indices m_interrupts;

};

