#include "orthocam.h"

OrthoCam::OrthoCam(float wmm, float hmm, GeoProjection* p)
  : Camera(p, hmm) {
  // set scale half-way between min & max in log scale
  m_scale = pow(10, .5 * (log10(minScale()) + log10(maxScale())));
  // temporary projection to keep aspect ratio
  m_projection.setToIdentity();
  m_projection.ortho(-wmm, wmm, -hmm, hmm, -1., 1.);
  reset(WGS84Point::fromLL(0, 0), Angle::fromRadians(0));
  resize(wmm, hmm);
}

// set projection from vp & resize the mmHeight accordingly
OrthoCam::OrthoCam(const QSizeF &vp, const WGS84Point& eye, quint32 scale, GeoProjection *proj)
  : Camera(proj, 0) {

  m_geoprojection->setReference(eye);
  m_northAngle = Angle::fromDegrees(0.);
  m_reference_0 = m_geoprojection->reference();
  m_northAngle_0 = m_northAngle;

  m_scale = scale;

  const qreal dx = .5 * vp.width();
  const qreal dy = .5 * vp.height();

  m_projection.setToIdentity();
  m_projection.ortho(-dx, dx, -dy, dy, -1., 1.);

  const WGS84Point p1 = m_geoprojection->toWGS84(QPoint(0., dy));

  auto d = (p1 - m_geoprojection->reference()).meters();

  m_mmHeight = 2000. * d / m_scale;

}

void OrthoCam::setScale(quint32 scale) {
  m_scale = scale;
  updateProjection();
}

void OrthoCam::updateProjection() {
  // distance in meters from center to the top edge of the display
  const float d = m_scale * m_mmHeight / 2000.;
  auto b = WGS84Bearing::fromMeters(d, Angle::fromDegrees(0.));
  const QPointF p1 = m_geoprojection->fromWGS84(m_geoprojection->reference() + b);
  const float y1 = p1.y();
  const float x1 = y1 * aspect();
  m_projection.setToIdentity();
  m_projection.ortho(-x1, x1, -y1, y1, -1., 1.);
}

quint32 OrthoCam::maxScale() const {return MAX_SCALE;}
quint32 OrthoCam::minScale() const {return MIN_SCALE;}

void OrthoCam::resize(float wmm, float hmm) {
  m_mmHeight = hmm;
  const float d = m_scale * m_mmHeight / 2000.;
  auto b = WGS84Bearing::fromMeters(d, Angle::fromDegrees(0.));
  const QPointF p1 = m_geoprojection->fromWGS84(m_geoprojection->reference() + b);
  const float y1 = p1.y();
  const float x1 = y1 * wmm / hmm;
  m_projection.setToIdentity();
  m_projection.ortho(-x1, x1, -y1, y1, -1., 1.);
}


void OrthoCam::reset(WGS84Point eye, Angle a) {
  m_geoprojection->setReference(eye);
  m_northAngle = a;
  m_reference_0 = eye;
  m_northAngle_0 = m_northAngle;
  doReset();
}

void OrthoCam::reset() {
  m_geoprojection->setReference(m_reference_0);
  m_northAngle = m_northAngle_0;
  doReset();
}

void OrthoCam::doReset() {
  m_view.setToIdentity();
  const float ca = m_northAngle.cos();
  const float sa = m_northAngle.sin();
  m_view.setRow(IX, QVector2D(ca, -sa));
  m_view.setRow(IY, QVector2D(sa, ca));
}

void OrthoCam::rotateEye(Angle a) {
  m_northAngle = m_northAngle + a;
  doReset();
}

WGS84Point OrthoCam::eye() const {return m_geoprojection->reference();}

Angle OrthoCam::northAngle() const {return m_northAngle;}


void OrthoCam::pan(QPointF /*dragStart*/, QPointF dragAmount) {
  auto c = m_geoprojection->toWGS84(QPointF(-dragAmount.x() / m_projection(0, 0),
                                            -dragAmount.y() / m_projection(1, 1)));
  m_geoprojection->setReference(c);
  doReset();
}

WGS84Point OrthoCam::location(const QPointF &cp) const {
  const QVector4D q(cp.x() / m_projection(0, 0),
                    cp.y() / m_projection(1, 1),
                    0.,
                    1.);
  const QVector4D p = m_view.transposed() * q;
  return m_geoprojection->toWGS84(QPointF(p.x(), p.y()));
}


