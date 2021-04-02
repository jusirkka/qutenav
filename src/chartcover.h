#pragma once
#include "types.h"
#include <QPointF>
#include <QVector>
#include "region.h"

using LLPolygon = QVector<WGS84PointVector>;

using PointVector = QVector<QPointF>;
using Polygon = QVector<PointVector>;

class GeoProjection;

class ChartCover {
public:

  ChartCover(const ChartCover&);
  ChartCover();
  ChartCover operator =(const ChartCover&);
  ChartCover(const LLPolygon& cov, const LLPolygon& nocov,
             const WGS84Point& sw, const WGS84Point& ne,
             const GeoProjection* gp);

  KV::Region region(const GeoProjection* gp) const;

private:

  static const int gridWidth = 21;

  KV::Region approximate(const PointVector& poly, const QRectF& box) const;
  bool isRectangle(const PointVector& poly) const;

  WGS84Point m_ref;
  KV::Region m_cover;
};
