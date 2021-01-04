#pragma once

#include <QMatrix4x4>
#include "camera.h"

class OrthoCam: public Camera {

public:

  OrthoCam(float wmm, float hmm, GeoProjection* proj);
  OrthoCam(const QSizeF& vp, const WGS84Point& eye, quint32 scale, GeoProjection* proj);

  void pan(QPointF dragStart, QPointF dragAmount) override;
  WGS84Point location(const QPointF &cp) const override;
  void rotateEye(Angle a) override;
  void setScale(quint32 scale) override;
  void resize(float wmm, float hmm) override;
  void reset() override;
  void reset(WGS84Point eye, Angle tilt) override;
  WGS84Point eye() const override;
  Angle northAngle() const override;
  quint32 maxScale() const override;
  quint32 minScale() const override;
  QRectF boundingBox() const override;



private:

  void updateProjection();
  void doReset();

  const quint32 MAX_SCALE = 50000000;
  const quint32 MIN_SCALE = 800;

  WGS84Point m_reference_0;
  Angle m_northAngle;
  Angle m_northAngle_0;
};
