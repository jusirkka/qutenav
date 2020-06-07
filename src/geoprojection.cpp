#include "geoprojection.h"
#include <QVector>
#include <QRectF>

GeoProjection* GeoProjection::CreateProjection(const QString& className) {
  if (className == "SimpleMercator") {
    return new SimpleMercator;
  }
  return nullptr;
}


void SimpleMercator::setReference(const WGS84Point &w) {
  GeoProjection::setReference(w);
  const double s = sin(m_ref.radiansLat());
  m_y30 = .5 * log((1 + s) / (1 - s)) * z0;
}

QPointF SimpleMercator::fromWGS84(const WGS84Point &p) const {
  const Angle dlng = Angle::fromDegrees(p.lng() - m_ref.lng());

  const double s = sin(p.radiansLat());
  const double y3 = .5 * log((1 + s) / (1 - s)) * z0;

  return QPointF(dlng.radians * z0, y3 - m_y30);
}

WGS84Point SimpleMercator::toWGS84(const QPointF &p) const {
  return WGS84Point::fromLLRadians(m_ref.radiansLng() + p.x() / z0,
                                   2.0 * atan(exp((m_y30 + p.y()) / z0)) - M_PI_2);
}

bool operator!= (const GeoProjection& p1, const GeoProjection& p2) {
  return p1.className() != p2.className();
}

bool operator== (const GeoProjection& p1, const GeoProjection& p2) {
  return !(p1 != p2);
}


QRectF findBoundingBox(const GeoProjection *p, const WGS84PointVector& points) {
  QPointF ur(-1.e15, -1.e15);
  QPointF ll(1.e15, 1.e15);
  for (const WGS84Point& point: points) {
    QPointF q = p->fromWGS84(point);
    ur.setX(qMax(ur.x(), q.x()));
    ur.setY(qMax(ur.y(), q.y()));
    ll.setX(qMin(ll.x(), q.x()));
    ll.setY(qMin(ll.y(), q.y()));
  }
  return QRectF(ll, ur); // inverted y-axis
}
