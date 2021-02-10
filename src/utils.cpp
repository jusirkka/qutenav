/* -*- coding: utf-8-unix -*-
 *
 * utils.cpp
 *
 * Created: 09/02/2021 2021 by Jukka Sirkka
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

#include "utils.h"

static const uint LEFT = 1;
static const uint RIGHT = 2;
static const uint BOTTOM = 4;
static const uint TOP = 8;

static uint locationCode(const glm::vec2& v, const QRectF& va) {
  uint code = 0;
  if (v.y > va.bottom()) {
    code |= TOP;
  } else if (v.y < va.top()) {
    code |= BOTTOM;
  }

  if (v.x > va.right()) {
    code |= RIGHT;
  } else if (v.x < va.left()) {
    code |= LEFT;
  }
  return code;
}

// Cohen-Sutherland test
bool outsideBox(const glm::vec2& v1, const glm::vec2& v2, const QRectF& va) {
  const uint c1 = locationCode(v1, va);
  const uint c2 = locationCode(v2, va);
  if (c1 == 0 && c2 == 0) return false;
  return (c1 & c2) != 0;
}



// https://wrf.ecse.rpi.edu//Research/Short_Notes/pnpoly.html
bool insidePolygon(uint count, uint offset, const glm::vec2* q,
                   const uint* indices, const QPointF& p) {
  bool c = false;
  const int n = count - 3;
  const int first = offset / sizeof(uint) + 1;
  for (int i0 = 0, j0 = n - 1; i0 < n; j0 = i0++) {
    auto i = indices[first + i0];
    auto j = indices[first + j0];
    if (((q[i].y > p.y()) != (q[j].y > p.y())) &&
        (p.x() < (q[j].x - q[i].x) * (p.y() - q[i].y) / (q[j].y - q[i].y) + q[i].x)) {
      c = !c;
    }
  }
  return c;
}

