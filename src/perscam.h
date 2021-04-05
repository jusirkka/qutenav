#pragma once

#include <QMatrix4x4>
#include "camera.h"

class PersCam: public Camera {

public:

  PersCam(float wmm, float hmm, GeoProjection* p);
  void pan(const QPointF& dragStart, const QPointF& dragAmount) override;
  WGS84Point location(const QPointF &cp) const override;
  QPointF position(const WGS84Point &wp) const override;
  void rotateEye(const Angle& a) override;
  void setScale(quint32 scale) override;
  void resize(float wmm, float hmm) override;
  void reset() override;
  void reset(const WGS84Point& eye, const Angle& tilt) override;
  void setEye(const WGS84Point& eye) override;
  WGS84Point eye() const override;
  Angle northAngle() const override;
  quint32 maxScale() const override;
  quint32 minScale() const override;
  QRectF boundingBox() const override;

  ~PersCam() = default;


private:

  float eyeDistFromScale() const;
  void setScaleFromEyeDist();
  void setEyeDist();

  void setEyeAndTilt(const WGS84Point& eye, const Angle& tilt);

protected:

  void doUpdate(const Camera *other) override;

private:


  const float EPSILON = 1.e-5;
  const float R0 = 6371008.8;
  const float PI = 3.14159265358979323846;
  const float FOV2 = PI / 8;
  const quint32 MIN_SCALE = 20000;

  QMatrix4x4 m_rot, m_rot0;
  QVector3D m_eye, m_eye0;
};
