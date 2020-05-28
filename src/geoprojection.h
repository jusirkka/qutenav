#pragma once

#include "types.h"
#include <QVector2D>

class GeoProjection {
public:

  static GeoProjection* CreateProjection(const QString& className);
  virtual WGS84Point toWGS84(const QVector2D& p) const = 0;
  virtual QVector2D fromWGS84(const WGS84Point& w) const = 0;
  virtual QString className() const = 0;

  virtual void setReference(const WGS84Point& w) {m_ref = w;}
  const WGS84Point& reference() const {return m_ref;}

protected:

  WGS84Point m_ref;

};


class SimpleMercator: public GeoProjection {
public:
  WGS84Point toWGS84(const QVector2D& p) const override;
  QVector2D fromWGS84(const WGS84Point& p) const override;
  QString className() const {return "SimpleMercator";}
  void setReference(const WGS84Point& w) override;

private:

  static constexpr double k0 = 0.9996;

  double m_y30;

};

bool operator!= (const GeoProjection& p1, const GeoProjection& p2);
bool operator== (const GeoProjection& p1, const GeoProjection& p2);
