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

void OrthoCam::setScale(quint32 scale) {
  m_scale = scale;
  updateProjection();
}

void OrthoCam::updateProjection() {
  // distance in meters from center to north edge of the display
  const float d = m_scale * m_mmHeight / 2000.;
  auto b = WGS84Bearing::fromMeters(d, northAngle());
  const QVector2D p1 = m_geoprojection->fromWGS84(m_geoprojection->reference() + b);
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
  auto b = WGS84Bearing::fromMeters(d, northAngle());
  const QVector2D p1 = m_geoprojection->fromWGS84(m_geoprojection->reference() + b);
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
  m_view.setRow(IX, QVector2D(ca, sa));
  m_view.setRow(IY, QVector2D(-sa, ca));

  // apparent horizontal scale changes with the reference point
  // FIXME/optimization: call only when needed, criterion based on derivatives of
  // geoprojection
  updateProjection();
}

void OrthoCam::rotateEye(Angle a) {
  m_view.setToIdentity();
  const float ca = a.cos();
  const float sa = a.sin();
  m_view.setRow(IX, QVector2D(ca, sa));
  m_view.setRow(IY, QVector2D(-sa, ca));
}

WGS84Point OrthoCam::eye() const {return m_geoprojection->reference();}

Angle OrthoCam::northAngle() const {return m_northAngle;}


void OrthoCam::pan(QVector2D /*dragStart*/, QVector2D dragAmount) {
  auto c = m_geoprojection->toWGS84(QVector2D(-dragAmount.x() / m_projection(0, 0),
                                              -dragAmount.y() / m_projection(1, 1)));
  m_geoprojection->setReference(c);
  doReset();
}


