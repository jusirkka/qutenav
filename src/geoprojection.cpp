#include "geoprojection.h"

GeoProjection* GeoProjection::CreateProjection(const QString& className) {
  if (className == "SimpleMercator") {
    return new SimpleMercator;
  }
  return nullptr;
}


void SimpleMercator::setReference(const WGS84Point &w) {
  GeoProjection::setReference(w);
  const double s = sin(m_ref.radiansLat());
  const double z = WGS84Point::semimajor_axis * k0;
  m_y30 = .5 * log((1 + s) / (1 - s)) * z;
}

QVector2D SimpleMercator::fromWGS84(const WGS84Point &p) const {
  const Angle dlng = Angle::fromDegrees(p.lng()) - Angle::fromDegrees(m_ref.lng());

  const double z = WGS84Point::semimajor_axis * k0;

  const double s = sin(p.radiansLat());
  const double y3 = .5 * log((1 + s) / (1 - s)) * z;

  return QVector2D(dlng.radians * z, y3 - m_y30);
}

WGS84Point SimpleMercator::toWGS84(const QVector2D &p) const {
  const double z = WGS84Point::semimajor_axis * k0;

  return WGS84Point::fromLLRadians(m_ref.radiansLng() + p.x() / z,
                                   2.0 * atan(exp((m_y30 + p.y()) / z)) - M_PI_2);
}
