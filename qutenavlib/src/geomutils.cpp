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

#include "geomutils.h"
#include <QDebug>

static const uint LEFT = 1;
static const uint RIGHT = 2;
static const uint BOTTOM = 4;
static const uint TOP = 8;
static const inline float eps = .1;

static uint locationCode(const glm::vec2& v, const QRectF& va) {
  uint code = 0;
  if (v.y > va.bottom() + eps) {
    code |= TOP;
  } else if (v.y < va.top() - eps) {
    code |= BOTTOM;
  }

  if (v.x > va.right() + eps) {
    code |= RIGHT;
  } else if (v.x < va.left() - eps) {
    code |= LEFT;
  }
  return code;
}

// https://en.wikipedia.org/wiki/Cohen%E2%80%93Sutherland_algorithm
bool crossesBox(const glm::vec2& v1, const glm::vec2& v2, const QRectF& va) {
  glm::vec2 p1 = v1;
  glm::vec2 p2 = v2;
  uint c1 = locationCode(p1, va);
  uint c2 = locationCode(p2, va);
  while (true) {
    // qDebug() << c1 << c2 << p1.x << p1.y << p2.x << p2.y << va.topLeft() << va.bottomRight();
    if (c1 == 0 || c2 == 0) return true; // at least one ep is inside
    if ((c1 & c2) != 0) return false;
    if ((c1 | c2) == (LEFT | RIGHT)) return true;
    if ((c1 | c2) == (TOP | BOTTOM)) return true;
    // failed triviality tests, so calculate the line segment to clip
    // from an outside point to an intersection with clip edge
    glm::vec2 p;
    // Both endpoints are outside the clip rectangle; pick one.
    uint c = c1 > c2 ? c1 : c2;
    // Now find the intersection point;
    // use formulas:
    //   slope = (y1 - y0) / (x1 - x0)
    //   x = x0 + (1 / slope) * (ym - y0), where ym is ymin or ymax
    //   y = y0 + slope * (xm - x0), where xm is xmin or xmax
    // No need to worry about divide-by-zero because, in each case, the
    // outcode bit being tested guarantees the denominator is non-zero
    if (c & TOP) { // point is above the clip window
      p.x = p1.x + (p2.x - p1.x) / (p2.y - p1.y) * (va.bottom() - p1.y);
      p.y = va.bottom();
    } else if (c & BOTTOM) { // point is below the clip window
      p.x = p1.x + (p2.x - p1.x) / (p2.y - p1.y) * (va.top() - p1.y);
      p.y = va.top();
    } else if (c & RIGHT) { // point is to the right of clip window
      p.y = p1.y + (p2.y - p1.y) / (p2.x - p1.x) * (va.right() - p1.x);
      p.x = va.right();
    } else if (c & LEFT) { // point is to the left of clip window
      p.y = p1.y + (p2.y - p1.y) / (p2.x - p1.x) * (va.left() - p1.x);
      p.x = va.left();
    }
    // Now we move outside point to intersection point to clip
    // and get ready for next pass.
    if (c == c1) {
      p1 = p;
      c1 = locationCode(p1, va);
    } else {
      p2 = p;
      c2 = locationCode(p2, va);
    }
  }
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


