#pragma once

#include "types.h"
#include <QPointF>

class GeoProjection {
public:

  static GeoProjection* CreateProjection(const QString& className);

  virtual WGS84Point toWGS84(const QPointF& p) const = 0;
  virtual QPointF fromWGS84(const WGS84Point& w) const = 0;
  virtual QString className() const = 0;

  virtual void setReference(const WGS84Point& w) {m_ref = w;}
  const WGS84Point& reference() const {return m_ref;}

  virtual ~GeoProjection() = default;

protected:

  WGS84Point m_ref;

};


class SimpleMercator: public GeoProjection {
public:
  WGS84Point toWGS84(const QPointF& p) const override;
  QPointF fromWGS84(const WGS84Point& p) const override;
  QString className() const {return "SimpleMercator";}
  void setReference(const WGS84Point& w) override;


private:

  static constexpr double z0 = WGS84Point::semimajor_axis * 0.9996;

  double m_y30;

};

bool operator!= (const GeoProjection& p1, const GeoProjection& p2);
bool operator== (const GeoProjection& p1, const GeoProjection& p2);

QRectF findBoundingBox(const GeoProjection* p, const WGS84PointVector& points);
