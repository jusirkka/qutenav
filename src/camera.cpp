#include "camera.h"


void Camera::update(const Camera *other) {
  doUpdate(other);
  m_projection = other->m_projection;
  m_view = other->m_view;
  m_scale = other->m_scale;
  m_geoprojection->setReference(other->geoprojection()->reference());
  m_mmHeight = other->m_mmHeight;
}
