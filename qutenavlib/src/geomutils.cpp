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

//
// Original source:
// https://github.com/paulhoux/Cinder-Samples/blob/master/GeometryShader/assets/shaders/lines1.geom
//

void thickerLines(const QVector<QPointF> &points, bool closed, qreal lw,
                  GL::VertexVector& vertices, GL::IndexVector& indices) {

  const GLfloat MITER_LIMIT = .75;

  const float HW = .5 * lw;

  GLuint offset = vertices.size() / 2;
  const int n = points.size() - 1;
  for (int i = 0; i < n; i++) {

    const glm::vec2 p0 = GL::Vec2(i == 0 ? (closed ? points[n - 1] : 2 * points[0] - points[1]) :
        points[i - 1]);
    const glm::vec2 p1 = GL::Vec2(points[i]);
    const glm::vec2 p2 = GL::Vec2(points[i + 1]);
    const glm::vec2 p3 = GL::Vec2(i == n - 1 ? (closed ? points[1] : points[n - 1] -  2 * points[n]) :
        points[i + 2]);

    // determine the direction of each of the 3 segments (previous, current, next)
    const glm::vec2 v0 = glm::normalize(p1 - p0);
    const glm::vec2 v1 = glm::normalize(p2 - p1);
    const glm::vec2 v2 = glm::normalize(p3 - p2);

    // determine the normal of each of the 3 segments (previous, current, next)
    const glm::vec2 n0 = glm::vec2(-v0.y, v0.x);
    const glm::vec2 n1 = glm::vec2(-v1.y, v1.x);
    const glm::vec2 n2 = glm::vec2(-v2.y, v2.x);

    // determine miter lines by averaging the normals of the 2 segments
    glm::vec2 miter_a = glm::normalize(n0 + n1);	// miter at start of current segment
    glm::vec2 miter_b = glm::normalize(n1 + n2);	// miter at end of current segment

    // determine the length of the miter from a right angled triangle (miter, n, v)
    GLfloat len_a = HW / glm::dot(miter_a, n1);
    GLfloat len_b = HW / glm::dot(miter_b, n1);

    glm::vec2 r;
    // prevent excessively long miters at sharp corners
    if (glm::dot(v0, v1) < - MITER_LIMIT) {
      miter_a = n1;
      len_a = HW;

      // close the gap
      if(glm::dot(v0, n1) > 0) {
        r = p1 + HW * n0;
        vertices << r.x << r.y;
        r = p1 + HW * n1;
        vertices << r.x << r.y;
        r = p1;
        vertices << r.x << r.y;
      } else {
        r = p1 - HW * n1;
        vertices << r.x << r.y;
        r = p1 - HW * n0;
        vertices << r.x << r.y;
        r = p1;
        vertices << r.x << r.y;
      }
      indices << offset << offset + 1 << offset + 2;
      offset += 3;
    }

    if (glm::dot(v1, v2) < -MITER_LIMIT ) {
      miter_b = n1;
      len_b = HW;
    }

    // generate the triangle strip
    r = p1 + len_a * miter_a;
    vertices << r.x << r.y;
    r = p1 - len_a * miter_a;
    vertices << r.x << r.y;
    r = p2 + len_b * miter_b;
    vertices << r.x << r.y;
    r = p2 - len_b * miter_b;
    vertices << r.x << r.y;

    indices << offset << offset + 1 << offset + 3
            << offset << offset + 3 << offset + 2;
    offset += 4;
  }
}

