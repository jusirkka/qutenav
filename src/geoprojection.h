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

  void setScaling(const QSizeF s) {m_scaling = s;}
  const QSizeF& scaling() const {return m_scaling;}

  GeoProjection()
    : m_ref(WGS84Point::fromLL(0., 0.))
    , m_scaling(1., 1.) {}

  virtual ~GeoProjection() = default;

protected:

  WGS84Point m_ref;
  QSizeF m_scaling;

};

class SimpleMercator: public GeoProjection {
public:
  SimpleMercator();
  WGS84Point toWGS84(const QPointF& p) const override;
  QPointF fromWGS84(const WGS84Point& p) const override;
  QString className() const {return "SimpleMercator";}
  void setReference(const WGS84Point& w) override;


private:

  static constexpr double z0 = WGS84Point::semimajor_axis * 0.9996;

  double m_y30;

};


class CM93Mercator: public GeoProjection {
public:
  CM93Mercator();
  WGS84Point toWGS84(const QPointF& p) const override;
  QPointF fromWGS84(const WGS84Point& p) const override;
  QString className() const {return "CM93Mercator";}
  void setReference(const WGS84Point& w) override;
  void setReference(const QPointF& p);

  // From opencpn / cm93.h
  static constexpr double zC = 6378388.0;

  static constexpr double scale = WGS84Point::semimajor_axis * 0.9996 / zC;

private:


  double m_y30;

};


bool operator!= (const GeoProjection& p1, const GeoProjection& p2);
bool operator== (const GeoProjection& p1, const GeoProjection& p2);

QRectF findBoundingBox(const GeoProjection* p, const WGS84PointVector& points);
