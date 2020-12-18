#include "camera.h"

QRectF Camera::boundingBox() const {
  const float dx = 1. / projection()(0, 0);
  const float dy = 1. / projection()(1, 1);

  if (northAngle().radians == 0.) {
    return QRectF(QPointF(- dx, - dy), QSizeF(2 * dx, 2 * dy)); // inverted y-axis
  }

  const QPointF p1(dx, 0.);
  const QPointF p2(0., dy);
  QPointF ur(-1.e15, -1.e15);
  QPointF ll(1.e15, 1.e15);
  const QVector<QPointF> ps {p1 + p2, p1 - p2, - p1 + p2, - p1 - p2};
  for (const QPointF& p: ps) {
    const QVector4D q = m_view * QVector4D(p);
    ur.setX(qMax(ur.x(), static_cast<double>(q.x())));
    ur.setY(qMax(ur.y(), static_cast<double>(q.y())));
    ll.setX(qMin(ll.x(), static_cast<double>(q.x())));
    ll.setY(qMin(ll.y(), static_cast<double>(q.y())));
  }
  return QRectF(ll, ur); // inverted y-axis
}


qreal Camera::distance(const QRectF &r) const {
  const QRectF b = boundingBox();
  if (!r.contains(b)) return 0.;
  qreal d = 1.e20;
  d = qMin(d, r.right() - b.right());
  d = qMin(d, b.left() - r.left());
  d = qMin(d, r.bottom() - b.bottom());
  d = qMin(d, b.top() - r.top());
  auto p = m_geoprojection->toWGS84(QPoint(0., d));
  return (p - eye()).meters();
}
