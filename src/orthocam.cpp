/* -*- coding: utf-8-unix -*-
 *
 * File: src/orthocam.cpp
 *
 * Copyright (C) 2021 Jukka Sirkka
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "orthocam.h"
#include <QVector2D>

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


void OrthoCam::reset(const WGS84Point& eye, const Angle& a) {
  m_geoprojection->setReference(eye);
  m_northAngle = a;
  m_reference_0 = eye;
  m_northAngle_0 = m_northAngle;
  doReset();
}

void OrthoCam::setEye(const WGS84Point& eye) {
  m_geoprojection->setReference(eye);
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

void OrthoCam::rotateEye(const Angle& a) {
  m_northAngle = m_northAngle + a;
  doReset();
}

WGS84Point OrthoCam::eye() const {return m_geoprojection->reference();}

Angle OrthoCam::northAngle() const {return m_northAngle;}


void OrthoCam::pan(const QPointF& /*dragStart*/, const QPointF& dragAmount) {
  auto c = m_geoprojection->toWGS84(QPointF(-dragAmount.x() / m_projection(0, 0),
                                            -dragAmount.y() / m_projection(1, 1)));
  m_geoprojection->setReference(c);
}

WGS84Point OrthoCam::location(const QPointF &cp) const {
  const QVector4D q(cp.x() / m_projection(0, 0),
                    cp.y() / m_projection(1, 1),
                    0.,
                    1.);
  const QVector4D p = m_view.transposed() * q;
  return m_geoprojection->toWGS84(QPointF(p.x(), p.y()));
}

QPointF OrthoCam::position(const WGS84Point& wp) const {
  const QPointF p = m_geoprojection->fromWGS84(wp);
  const QVector4D q = m_view * QVector4D(p.x(), p.y(), 0., 1.);
  return QPointF(m_projection(0, 0) * q.x(), m_projection(1, 1) * q.y());
}

QRectF OrthoCam::boundingBox() const {
  const float dx = 1. / projection()(0, 0);
  const float dy = 1. / projection()(1, 1);

  if (northAngle().isZero()) {
    return QRectF(QPointF(- dx, - dy), QSizeF(2 * dx, 2 * dy)); // inverted y-axis
  }

  const QPointF p1(dx, 0.);
  const QPointF p2(0., dy);
  QPointF ur(-1.e15, -1.e15);
  QPointF ll(1.e15, 1.e15);
  const QVector<QPointF> ps {p1 + p2, p1 - p2, - p1 + p2, - p1 - p2};
  for (const QPointF& p: ps) {
    const QVector4D q = m_view * QVector4D(p);
    ur.setX(qMax(ur.x(), static_cast<qreal>(q.x())));
    ur.setY(qMax(ur.y(), static_cast<qreal>(q.y())));
    ll.setX(qMin(ll.x(), static_cast<qreal>(q.x())));
    ll.setY(qMin(ll.y(), static_cast<qreal>(q.y())));
  }
  return QRectF(ll, ur); // inverted y-axis
}

void OrthoCam::doUpdate(const Camera *other) {
  auto cam = dynamic_cast<const OrthoCam*>(other);
  if (!cam) return;
  m_northAngle = cam->m_northAngle;
}
