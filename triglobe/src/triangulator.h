#pragma once

#include <QStack>
#include <QFile>
#include <QList>
#include "nanoflann.hpp"

#include "precision.h"

namespace SD {
class Function;
}

class Triangulator {
public:
  Triangulator(const SD::Function* scene, scalar err, int ec);
  void triangulate();
  void write(QFile& f);

private:

  using Mesh = QVector<vec3>;
  using Indices = QVector<quint32>;

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
        : mesh(m) {}

      inline unsigned int kdtree_get_point_count() const {
        return indices.size();
      }
      inline float kdtree_get_pt(const unsigned int i, int dim) const {
        return mesh[indices[i]][dim];
      }
      template <class BBOX> bool kdtree_get_bbox(BBOX&) const {
        return false;
      }

      const Mesh& mesh;
      Indices indices;
    };

    using Front_pt_kdtree = nanoflann::KDTreeSingleIndexDynamicAdaptor
    <nanoflann::L2_Simple_Adaptor<scalar, Adaptor>, Adaptor, 3>;


    bool canExpand() const;
    quint32 meshIndex(qint32 diff) const;
    quint32 meshIndex(quint32 base, qint32 diff) const;
    quint32 frontIndex(qint32 diff) const;
    quint32 frontIndex(quint32 base, qint32 diff) const;
    scalar expansionAngle() const;
    scalar computeOpeningAngle(quint32 fi, quint32 mi) const;
    void computeAngles();
    quint32 checkInterCollision(const Front* target, scalar radius, bool& found) const;
    quint32 checkIntraCollision(scalar radius, bool& found) const;
    void deleteCurrent();
    void removeFromTree(quint32 mi);
    void addToTree(const DataList& d);
    void addToTree(quint32 mi);

    quint32 currentIndex;
    DataList points;
    Adaptor adaptor;
    Front_pt_kdtree db;

  };

  using FrontStack = QStack<Front*>;

  Front* seedHexagon();
  void expand(Front* f, scalar edgeLen);
  void expand(Front* f);

  void splitFront(Front* f, quint32 i1, quint32 i2);
  void joinFronts(Front* f1, const Front* f2, quint32 i1, quint32 i2);

  scalar rocToEdgeLen(scalar roc) const;
  scalar flatRegionEdgeLen(const Front* f) const;
  scalar flatRegionEdgeLen(const vec3& p, const vec3& x, const vec3& y, scalar oa) const;

  const SD::Function* m_scene;
  FrontStack m_fronts;
  Mesh m_mesh;
  Indices m_triangles;
  scalar m_err;
  int m_expandCount;

};

