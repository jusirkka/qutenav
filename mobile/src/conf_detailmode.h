#pragma once

#include "configgroup.h"


namespace Conf {

class DetailMode: public ConfigGroup {
public:

  static DetailMode *self();
  ~DetailMode();

  CONF_DECL(widthMM, WidthMM, width_mm, double, toDouble)
  CONF_DECL(heightMM, HeightMM, height_mm, double, toDouble)
  CONF_DECL(name, Name, name, QString, toString)
  CONF_DECL(projection, Projection, projection, QString, toString)
  CONF_DECL(scale, Scale, scale, uint, toUInt)
  CONF_DECL(eye, Eye, eye, QString, toString)
  CONF_DECL(northAngle, NorthAngle, north_angle, double, toDouble)

private:

  DetailMode();

};

}


