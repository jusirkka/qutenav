#pragma once

#include "types.h"

#include <QMatrix4x4>
#include <QVector2D>

class ScaleOutOfBounds {
public:
  ScaleOutOfBounds(quint32 scale): m_suggestedScale(scale) {}
  quint32 suggestedScale() const {return m_suggestedScale;}
private:
  quint32 m_suggestedScale;
};



class Camera {
public:

  // drag start & end in normalized device coordinates
  virtual void pan(QVector2D dragStart, QVector2D dragAmount) = 0;
  virtual void rotateEye(Angle angle) = 0;
  virtual void setScale(quint32 scale) = 0;
  virtual void resize(float wmm, float hmm) = 0;
  virtual void reset() = 0;
  virtual void reset(WGS84Point eye, Angle tilt) = 0;
  virtual WGS84Point eye() const = 0;
  virtual Angle northAngle() const = 0;
  virtual quint32 maxScale() const = 0;
  virtual quint32 minScale() const = 0;

  const QMatrix4x4& projection() const {return m_projection;}
  const QMatrix4x4& view() const {return m_view;}
  quint32 scale() const {return m_scale;}

protected:

  const int IX = 0;
  const int IY = 1;
  const int IZ = 2;

  QMatrix4x4 m_projection;
  QMatrix4x4 m_view;
  quint32 m_scale;

};
