#include "camera.h"

QRectF Camera::boundingBox() const {
  // distance in meters from center to the top edge of the display
  const float d1 = m_scale * m_mmHeight / 2000.;
  auto b1 = WGS84Bearing::fromMeters(d1, northAngle());
  const QPointF p1 = m_geoprojection->fromWGS84(m_geoprojection->reference() + b1);
  // distance in meters from center to the right edge of the display
  const float d2 = m_scale * m_mmHeight / 2000. * aspect();
  auto b2 = WGS84Bearing::fromMeters(d2, northAngle() + Angle::fromDegrees(90));
  const QPointF p2 = m_geoprojection->fromWGS84(m_geoprojection->reference() + b2);
  QVector<QPointF> ps{p1 + p2, p1 - p2, - p1 + p2, - p1 - p2};
  QPointF ur(-1.e15, -1.e15);
  QPointF ll(1.e15, 1.e15);
  for (const QPointF& p: ps) {
    ur.setX(qMax(ur.x(), p.x()));
    ur.setY(qMax(ur.y(), p.y()));
    ll.setX(qMin(ll.x(), p.x()));
    ll.setY(qMin(ll.y(), p.y()));
  }
  return QRectF(ll, ur); // inverted y-axis
}
