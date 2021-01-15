#pragma once

#include "types.h"

class Tessellator {
public:
  virtual void addPolygon(uint offset, size_t count) = 0;
  virtual void addHole(uint offset, size_t count) = 0;
  virtual GL::IndexVector triangulate() = 0;

  virtual ~Tessellator() = default;

};
