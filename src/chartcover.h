#pragma once
#include "types.h"
#include <QPointF>
#include <QVector>

using Region = QVector<WGS84PointVector>;

using PointVector = QVector<QPointF>;
using PRegion = QVector<PointVector>;

class GeoProjection;

class ChartCover {
public:

  ChartCover(const ChartCover&);
  ChartCover();
  ChartCover operator =(const ChartCover&);
  ChartCover(const Region& cov, const Region& nocov, const GeoProjection* proj);

  bool covers(const QPointF& p, const GeoProjection* gp) const;

private:

  WGS84Point m_ref;
  PRegion m_cov;
  PRegion m_nocov;

};
