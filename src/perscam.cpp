#include "perscam.h"
#include <cmath>
#include <QVector2D>

PersCam::PersCam(float wmm, float hmm, GeoProjection* p)
  : Camera(p, hmm)
{
  // set scale half-way between min & max in log scale
  m_scale = pow(10, .5 * (log10(minScale()) + log10(maxScale())));
  reset(WGS84Point::fromLL(0, 0), Angle::fromRadians(0));
  resize(wmm, hmm);
}

void PersCam::resize(float wmm, float hmm) {
  m_projection.setToIdentity();
  m_projection.perspective(FOV2 * 2 * 180 / PI, wmm / hmm, 1.e-4, 10);
  m_mmHeight = hmm;
  // eye dist depends on scale and mm_height
  setEyeDist();
}

void PersCam::setScale(quint32 scale) {
  m_scale = scale;
  // eye dist depends on scale and mm_height
  setEyeDist();
}


void PersCam::setEyeDist() {
  if (m_scale < minScale()) throw ScaleOutOfBounds(minScale());
  if (m_scale > maxScale()) throw ScaleOutOfBounds(maxScale());
  m_eye = eyeDistFromScale() * m_eye.normalized();
  m_view.setToIdentity();
  m_view.translate(- m_eye);
  m_view = m_rot * m_view;
}



void PersCam::reset(WGS84Point e, Angle a) {

  m_geoprojection->setReference(e);

  const float cx = cos(e.radiansLng());
  const float sx = sin(e.radiansLng());
  const float cy = cos(e.radiansLat());
  const float sy = sin(e.radiansLat());

  QVector3D z(cx * cy,
              sx * cy,
                   sy);
  if (1 - std::abs(z[2]) < EPSILON) {
    const float sy1 = 1 - EPSILON;
    const float cy1 = sqrt(1 - sy1 * sy1);
    if (e.northern()) {
      z = QVector3D(cx * cy1,
                    sx * cy1,
                         sy1);
    } else {
      z = QVector3D(cx * cy1,
                    sx * cy1,
                       - sy1);
    }
  }

  const QVector3D y = QVector3D(-z[2] * z[0], -z[2] * z[1], 1 - z[2] * z[2]).normalized();


  m_rot.setToIdentity();
  m_rot.setRow(IX, QVector3D::crossProduct(y, z));
  m_rot.setRow(IY, y);
  m_rot.setRow(IZ, z);

  QMatrix4x4 r;
  r.setToIdentity();
  const float ca = cos(a.radians);
  const float sa = sin(a.radians);
  r.setRow(IX, QVector2D(ca, -sa));
  r.setRow(IY, QVector2D(sa, ca));

  m_rot = r * m_rot;
  m_rot0 = m_rot;

  m_eye = eyeDistFromScale() * z;
  m_eye0 = m_eye;

  m_view.setToIdentity();
  m_view.translate(- m_eye);
  m_view = m_rot * m_view;
}

void PersCam::reset() {
  m_rot = m_rot0;
  m_eye = m_eye0;

  m_geoprojection->setReference(eye());

  setScaleFromEyeDist();

  m_view.setToIdentity();
  m_view.translate(- m_eye);
  m_view = m_rot * m_view;
}


float PersCam::eyeDistFromScale() const {
  const float phi2 = m_scale * m_mmHeight * 0.001 / R0 / 2;
  return sin(phi2 + FOV2) / sin(FOV2);
}


void PersCam::setScaleFromEyeDist() {
  const float phi2 = asin(m_eye.length() * sin(FOV2)) - FOV2;
  m_scale = phi2 * R0 * 2 / m_mmHeight * 1000;
}

void PersCam::rotateEye(Angle a) {

  QMatrix4x4 r;
  r.setToIdentity();
  const float ca = cos(a.radians);
  const float sa = sin(a.radians);
  r.setRow(IX, QVector2D(ca, -sa));
  r.setRow(IY, QVector2D(sa, ca));

  m_rot = r * m_rot;

  m_view.setToIdentity();
  m_view.translate(- m_eye);
  m_view = m_rot * m_view;
}

quint32 PersCam::maxScale() const {
  return (PI/2 - FOV2) * R0 * 2 / m_mmHeight * 1000;
}

quint32 PersCam::minScale() const {
  return MIN_SCALE;
}



WGS84Point PersCam::eye() const {
  const float lng = atan2(m_eye[1], m_eye[0]);
  const float lat = asin(m_eye[2] / m_eye.length());
  return WGS84Point::fromLLRadians(lng, lat);
}

Angle PersCam::northAngle() const {
  const QVector3D z = m_eye.normalized();
  const QVector3D y = QVector3D(-z[2] * z[0], -z[2] * z[1], 1 - z[2] * z[2]).normalized();

  QMatrix4x4 baseRot;
  baseRot.setToIdentity();
  baseRot.setRow(IX, QVector3D::crossProduct(y, z));
  baseRot.setRow(IY, y);
  baseRot.setRow(IZ, z);

  QMatrix4x4 eyeRot = m_rot * baseRot.transposed();

  return Angle::ATan2(eyeRot.row(IY)[0], eyeRot.row(IY)[1]);
}

void PersCam::pan(QPointF dragStart, QPointF dragAmount) {
  const float a = m_projection(IY, IY) / m_projection(IX, IX);
  const float D = m_eye.length();
  const float c = m_projection(IY, IY);
  const QVector2D p = sqrt(D * D - 1) / c * QVector2D(a * dragStart.x(), dragStart.y());
  const QVector2D d = sqrt(D * D - 1) / c * QVector2D(a * dragAmount.x(), dragAmount.y());
  const QVector2D x(1, 0);
  QVector2D p0 = x;
  if (p.length() > 0) {
    p0 = p.normalized();
  }
  const QVector2D d0 = d.normalized();

  QMatrix4x4 r1;
  r1.setToIdentity();
  const float c1 = x[IX] * p0[IX] + x[IY] * p0[IY];
  const float s1 = x[IX] * p0[IY] - x[IY] * p0[IX];
  r1.setRow(IX, QVector2D(c1, -s1));
  r1.setRow(IY, QVector2D(s1, c1));

  auto clamp = [](float x) {return qMax(qMin(x, 1.f), 0.f);};
  QMatrix4x4 r2;
  r2.setToIdentity();
  const float c2 = cos(PI / 2 * clamp(p.length()));
  const float s2 = sin(PI / 2 * clamp(p.length()));
  r2.setRow(IX, QVector3D(c2, 0, s2));
  r2.setRow(IZ, QVector3D(-s2, 0, c2));

  QMatrix4x4 r3;
  r3.setToIdentity();
  const float c3 = p0[IX] * d0[IX] + p0[IY] * d0[IY];
  const float s3 = p0[IX] * d0[IY] - p0[IY] * d0[IX];
  r3.setRow(IX, QVector2D(c3, -s3));
  r3.setRow(IY, QVector2D(s3, c3));

  const QVector3D axis = r3 * r2 * r1 * QVector3D(0, 1, 0);

  const float phi = 90 * clamp(d.length()) * (1 - clamp(p.length()) * (1 - std::abs(c1)));

  QMatrix4x4 r;
  r.setToIdentity();
  r.rotate(phi, axis);

  m_rot = r * m_rot;

  m_eye = eyeDistFromScale() * m_rot.row(IZ).toVector3D();

  m_geoprojection->setReference(eye());

  m_view.setToIdentity();
  m_view.translate(- m_eye);
  m_view = m_rot * m_view;
}


