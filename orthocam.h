#pragma once

#include <QMatrix4x4>
#include "camera.h"

class OrthoCam: public Camera {

public:

  OrthoCam(float wmm, float hmm);
  void pan(QVector2D dragStart, QVector2D dragAmount) override;
  void rotateEye(Angle a) override;
  void setScale(quint32 scale) override;
  void resize(float wmm, float hmm) override;
  void reset() override;
  void reset(WGS84Point eye, Angle tilt) override;
  WGS84Point eye() const override;
  Angle northAngle() const override;
  quint32 maxScale() const override;
  quint32 minScale() const override;



private:


};
