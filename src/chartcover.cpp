#include "chartcover.h"
#include "geoprojection.h"

ChartCover::ChartCover(const Region& cov,
                       const Region& nocov,
                       const GeoProjection* proj)
  : m_ref(proj->reference()) {

  for (const WGS84PointVector& vs: cov) {
    PointVector ps;
    for (auto v: vs) {
      ps << proj->fromWGS84(v);
    }
    m_cov.append(ps);
  }

  for (const WGS84PointVector& vs: nocov) {
    PointVector ps;
    for (auto v: vs) {
      ps << proj->fromWGS84(v);
    }
    m_nocov.append(ps);
  }
}

// https://wrf.ecse.rpi.edu//Research/Short_Notes/pnpoly.html
static bool inpolygon(const PointVector& ps, const QPointF& p) {
  bool c = false;
  const int n = ps.size();
  for (int i = 0, j = n - 1; i < n; j = i++) {
    if (((ps[i].y() > p.y()) != (ps[j].y() > p.y())) &&
        (p.x() < (ps[j].x() - ps[i].x()) * (p.y() - ps[i].y()) / (ps[j].y() - ps[i].y()) + ps[i].x())) {
      c = !c;
    }
  }
  return c;
}


bool ChartCover::covers(const QPointF &p, const GeoProjection *gp) const {
  const QPointF q = p - gp->fromWGS84(m_ref);
  for (const PointVector& ps: m_nocov) {
    if (inpolygon(ps, q)) return false;
  }
  for (const PointVector& ps: m_cov) {
    if (inpolygon(ps, q)) return true;
  }
  return false;
}
