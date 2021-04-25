/**********************************************************************
 *
 * GEOS - Geometry Engine Open Source
 * http://geos.osgeo.org
 *
 * Copyright (C) 2009 2011 Sandro Santilli <strk@kbt.io>
 * Copyright (C) 2005-2006 Refractions Research Inc.
 * Copyright (C) 2001-2002 Vivid Solutions Inc.
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU Lesser General Public Licence as published
 * by the Free Software Foundation.
 * See the COPYING file for more information.
 *
 **********************************************************************
 *
 * Last port: geom/LineSegment.java r18 (JTS-1.11)
 *
 *********************************************************************

 Adapted for QOpenCPN (c) 2021 Jukka Sirkka

*/

#include "LineSegment.h"
#include "Coordinate.h"

#include <algorithm> // for max
#include <cmath>

using Distance = geos::algorithm::Distance;

namespace geos {
namespace geom { // geos::geom


/*public*/
LineSegment LineSegment::reverse() const
{
  return LineSegment(m_p1, m_p0);
}

/*public*/
double
LineSegment::projectionFactor(const Coordinate& p) const
{
  if (p == m_p0) return 0.0;
  if (p == m_p1) return 1.0;

  // Otherwise, use comp.graphics.algorithms Frequently Asked Questions method
  /*(1)     	      AC dot AB
                   r = ---------
                         ||AB||^2
                r has the following meaning:
                r=0 P = A
                r=1 P = B
                r<0 P is on the backward extension of AB
                r>1 P is on the forward extension of AB
                0<r<1 P is interior to AB
        */

  const Coordinate p01 = m_p1 - m_p0;
  return glm::dot(p - m_p0, p01) / glm::dot(p01, p01);
}

/*public*/
double
LineSegment::segmentFraction(const Coordinate& inputPt) const
{
  double segFrac = projectionFactor(inputPt);
  if(segFrac < 0.0) {
    segFrac = 0.0;
  }
  else if(segFrac > 1.0) {
    segFrac = 1.0;
  }
  return segFrac;
}

/*public*/
void
LineSegment::project(const Coordinate& p, Coordinate& ret) const
{
  if (p == m_p0 || p == m_p1) {
    ret = p;
  }
  double r = projectionFactor(p);
  ret = m_p0 + r * (m_p1 - m_p0);
}

bool
LineSegment::project(const LineSegment& seg, LineSegment& ret) const
{
  double pf0 = projectionFactor(seg[0]);
  double pf1 = projectionFactor(seg[1]);
  // check if segment projects at all

  if (pf0 >= 1.0 && pf1 >= 1.0) {
    return false;
  }

  if (pf0 <= 0.0 && pf1 <= 0.0) {
    return false;
  }

  Coordinate newp0;
  project(seg[0], newp0);
  Coordinate newp1;
  project(seg[1], newp1);

  ret = LineSegment(newp0, newp1);

  return true;
}

void LineSegment::closestPoint(const Coordinate& p, Coordinate& ret) const {
  const double factor = projectionFactor(p);
  if (factor > 0 && factor < 1) {
    project(p, ret);
    return;
  }

  const double dist0 = glm::distance(p, m_p0);
  const double dist1 = glm::distance(p, m_p1);

  if (dist0 < dist1) {
    ret = m_p0;
  } else {
    ret = m_p1;
  }
}


static int compare(const Coordinate& p1, const Coordinate& p2) {
  if (p1.x < p2.x) {
    return -1;
  }
  if (p1.x > p2.x) {
    return 1;
  }
  if (p1.y < p2.y) {
    return -1;
  }
  if (p1.y > p2.y) {
    return 1;
  }
  return 0;
}

/*public*/
int
LineSegment::compareTo(const LineSegment& other) const {
  int comp0 = compare(m_p0, other[0]);
  if (comp0 != 0) {
    return comp0;
  }
  return compare(m_p1, other[1]);
}

bool LineSegment::equalsTopo(const LineSegment& other) const {
  return (m_p0 == other[0] && m_p1 == other[1]) ||  (m_p0 == other[1] && m_p1 == other[0]);
}




/* public */
void
LineSegment::pointAlongOffset(double segmentLengthFraction,
                              double offsetDistance,
                              Coordinate& ret) const {
  // the point on the segment line
  const Coordinate delta = m_p1 - m_p0;
  const Coordinate seg = m_p0 + segmentLengthFraction * delta;

  if (offsetDistance == 0.) {
    ret = seg;
    return;
  }

  const double len = glm::length(delta);

  if (len < geom::eps) {
    throw util::IllegalStateException("Cannot compute offset from zero-length line segment");
  }

  const Coordinate u(- delta.y, delta.x);

  // the offset point is the seg point plus the offset
  // vector rotated 90 degrees CCW
  ret = seg + offsetDistance / len * u;
}


} // namespace geos::geom
} // namespace geos
