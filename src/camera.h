#pragma once

#include "types.h"
#include "geoprojection.h"

#include <QMatrix4x4>
#include <QPointF>
#include <QRectF>

class ScaleOutOfBounds {
public:
  ScaleOutOfBounds(quint32 scale): m_suggestedScale(scale) {}
  quint32 suggestedScale() const {return m_suggestedScale;}
private:
  quint32 m_suggestedScale;
};



class Camera {
public:

  // drag start & end in clip coordinates
  virtual void pan(const QPointF& dragStart, const QPointF& dragAmount) = 0;
  virtual void rotateEye(const Angle& angle) = 0;
  virtual void setScale(quint32 scale) = 0;
  virtual void resize(float wmm, float hmm) = 0;
  virtual void reset() = 0;
  virtual void reset(const WGS84Point& eye, const Angle& tilt) = 0;
  virtual void setEye(const WGS84Point& eye) = 0;
  virtual WGS84Point eye() const = 0;
  virtual Angle northAngle() const = 0;
  virtual quint32 maxScale() const = 0;
  virtual quint32 minScale() const = 0;
  // map from clip space to WGS84
  virtual WGS84Point location(const QPointF& cp) const = 0;
  // map from WGS84 to clip space
  virtual QPointF position(const WGS84Point& wp) const = 0;

  virtual QRectF boundingBox() const = 0;

  const QMatrix4x4& projection() const {return m_projection;}
  const QMatrix4x4& view() const {return m_view;}
  quint32 scale() const {return m_scale;}
  const GeoProjection* geoprojection() const {return m_geoprojection;}
  float heightMM() const {return m_mmHeight;}
  float aspect() const {
    return m_projection(1, 1) / m_projection(0, 0);
  }

  void update(const Camera* other);

  virtual ~Camera() {delete m_geoprojection;}

protected:

  virtual void doUpdate(const Camera* other) = 0;

  Camera(GeoProjection* proj, float hmm)
    : m_geoprojection(proj)
    , m_mmHeight(hmm) {}

  const int IX = 0;
  const int IY = 1;
  const int IZ = 2;

  QMatrix4x4 m_projection;
  QMatrix4x4 m_view;
  quint32 m_scale;
  GeoProjection* m_geoprojection;
  float m_mmHeight;
};
