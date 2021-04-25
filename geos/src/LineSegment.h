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

#pragma once

#include <memory> // for unique_ptr


#include <cassert>
#include <cmath> // for atan2

#include <QPointF>
#include <QRectF>

#include "Distance.h"

// Forward declarations
namespace geos {
namespace geom {
class LineString;
}
}

namespace geos {
namespace geom { // geos::geom

/**
 * Represents a line segment defined by two Coordinate.
 * Provides methods to compute various geometric properties
 * and relationships of line segments.
 *
 * This supports a common pattern of reusing a single LineSegment
 * object as a way of computing segment properties on the
 * segments defined by arrays or lists of {@link Coordinate}s.

 * This class is immutable.
 *
 */

class LineSegment {
private:

  Coordinate m_p0; /// Segment start
  Coordinate m_p1; /// Segment end

  QRectF m_envelope;
  qreal m_dist;
  qreal m_angle;
  qreal m_bearing;

public:


  LineSegment();

  /// Constructs a LineSegment with the given start and end Coordinates.
  LineSegment(const Coordinate& c0, const Coordinate& c1);
  LineSegment(double x0, double y0, double x1, double y1);


  const Coordinate& operator[](std::size_t i) const;


  /// Returns the length of the line segment.
  double length() const {return m_dist;}

  /// Tests whether the segment is horizontal.
  ///
  /// @return <code>true</code> if the segment is horizontal
  ///
  bool isHorizontal() const;

  /// Tests whether the segment is vertical.
  ///
  /// @return <code>true</code> if the segment is vertical
  ///
  bool isVertical() const;


  /// Reverses the direction of the line segment.
  LineSegment reverse() const;

  /// @return the angle this segment makes with the x-axis (in radians)
  double angle() const;
  double bearing() const;
  const QRectF& envelope() const {return m_envelope;}

  int index(const Coordinate& p) const;

  /// Computes the midpoint of the segment
  //
  /// @param ret will be set to the midpoint of the segment
  ///
  void midPoint(Coordinate& ret) const;

  /// Computes the distance between this line segment and another one.
  double distance(const LineSegment& ls) const;

  /// Computes the distance between this line segment and a point.
  double distance(const Coordinate& p) const;

  /** \brief
     * Computes the perpendicular distance between the (infinite)
     * line defined by this line segment and a point.
     */
  double distancePerpendicular(const Coordinate& p) const;

  /** \brief
     * Computes the Coordinate that lies a given
     * fraction along the line defined by this segment.
     *
     * A fraction of <code>0.0</code> returns the start point of
     * the segment; a fraction of <code>1.0</code> returns the end
     * point of the segment.
     * If the fraction is < 0.0 or > 1.0 the point returned
     * will lie before the start or beyond the end of the segment.
     *
     * @param segmentLengthFraction the fraction of the segment length
     *        along the line
     * @param ret will be set to the point at that distance
     */
  void pointAlong(double segmentLengthFraction, Coordinate& ret) const;

  /** \brief
     * Computes the {@link Coordinate} that lies a given
     * fraction along the line defined by this segment and offset from
     * the segment by a given distance.
     *
     * A fraction of <code>0.0</code> offsets
     * from the start point of the segment;
     * a fraction of <code>1.0</code> offsets
     * from the end point of the segment.
     *
     * The computed point is offset to the left of the line
     * if the offset distance is positive, to the right if negative.
     *
     * @param segmentLengthFraction the fraction of the segment
     *                              length along the line
     *
     * @param offsetDistance the distance the point is offset
     *        from the segment
     *         (positive is to the left, negative is to the right)
     *
     * @param ret will be set to the point at that distance and offset
     *
     * @throws IllegalStateException if the segment has zero length
     */
  void pointAlongOffset(double segmentLengthFraction,
                        double offsetDistance,
                        Coordinate& ret) const;

  /** \brief
     * Compute the projection factor for the projection of the point p
     * onto this LineSegment.
     *
     * The projection factor is the constant r
     * by which the vector for this segment must be multiplied to
     * equal the vector for the projection of p on the line
     * defined by this segment.
     *
     * The projection factor returned will be in the range
     * (-inf, +inf)
     *
     * @param p the point to compute the factor for
     *
     * @return the projection factor for the point
     *
     */
  double projectionFactor(const Coordinate& p) const;

  /** \brief
     * Computes the fraction of distance (in <tt>[0.0, 1.0]</tt>)
     * that the projection of a point occurs along this line segment.
     *
     * If the point is beyond either ends of the line segment,
     * the closest fractional value (<tt>0.0</tt> or <tt>1.0</tt>)
     * is returned.
     *
     * Essentially, this is the {@link #projectionFactor} clamped to
     * the range <tt>[0.0, 1.0]</tt>.
     *
     * @param inputPt the point
     * @return the fraction along the line segment the projection
     *         of the point occurs
     */
  double segmentFraction(const Coordinate& inputPt) const;

  /** \brief
     * Compute the projection of a point onto the line determined
     * by this line segment.
     *
     * Note that the projected point
     * may lie outside the line segment.  If this is the case,
     * the projection factor will lie outside the range [0.0, 1.0].
     */
  void project(const Coordinate& p, Coordinate& ret) const;

  /** \brief
     * Project a line segment onto this line segment and return the resulting
     * line segment.
     *
     * The returned line segment will be a subset of
     * the target line line segment.  This subset may be null, if
     * the segments are oriented in such a way that there is no projection.
     *
     * Note that the returned line may have zero length (i.e. the same endpoints).
     * This can happen for instance if the lines are perpendicular to one another.
     *
     * @param seg the line segment to project
     * @param ret the projected line segment
     * @return true if there is an overlap, false otherwise
     */
  bool project(const LineSegment& seg, LineSegment& ret) const;

  /// Computes the closest point on this line segment to another point.
  //
  /// @param p the point to find the closest point to
  /// @param ret the Coordinate to which the closest point on the line segment
  ///            to the point p will be written
  ///
  void closestPoint(const Coordinate& p, Coordinate& ret) const;

  /** \brief
     * Compares this object with the specified object for order.
     *
     * Uses the standard lexicographic ordering for the points in the LineSegment.
     *
     * @param  other  the LineSegment with which this LineSegment
     *            is being compared
     * @return a negative integer, zero, or a positive integer as this
     *         LineSegment is less than, equal to, or greater than the
     *         specified LineSegment
     */
  int compareTo(const LineSegment& other) const;

  /** \brief
     *  Returns <code>true</code> if <code>other</code> is
     *  topologically equal to this LineSegment (e.g. irrespective
     *  of orientation).
     *
     * @param  other  a <code>LineSegment</code> with which to do the comparison.
     * @return true if other is a LineSegment
     *      with the same values for the x and y ordinates.
     */
  bool equalsTopo(const LineSegment& other) const;


};


/// Checks if two LineSegment are equal (2D only check)
bool operator==(const LineSegment& a, const LineSegment& b);


inline
LineSegment::LineSegment(const Coordinate& c0, const Coordinate& c1)
  : m_p0(c0)
  , m_p1(c1)
  , m_envelope(QPointF(qMin(m_p0.x, m_p1.x), qMin(m_p0.y, m_p1.y)),
               QPointF(qMax(m_p0.x, m_p1.x), qMax(m_p0.y, m_p1.y)))
  , m_dist(glm::distance(m_p0, m_p1))
  , m_angle(std::atan2(m_p1.y - m_p0.y, m_p1.x - m_p0.x))
{
  m_bearing = 90 - 180. / M_PI * m_angle;
  while (m_bearing < 0) m_bearing += 360.;
}

inline
LineSegment::LineSegment(double x0, double y0, double x1, double y1)
  : LineSegment(Coordinate(x0, y0), Coordinate(x1, y1))
{}

inline
LineSegment::LineSegment()
  : LineSegment(Coordinate(0., 0.), Coordinate(0., 0.))
{}

inline double
LineSegment::distancePerpendicular(const Coordinate& p) const
{
  return algorithm::Distance::pointToLinePerpendicular(p, m_p0, m_p1);
}

inline void
LineSegment::pointAlong(double segmentLengthFraction, Coordinate& ret) const
{
  ret = m_p0 + segmentLengthFraction * (m_p1 - m_p0);
}

inline double
LineSegment::distance(const LineSegment& ls) const {
  return algorithm::Distance::segmentToSegment(m_p0, m_p1, ls[0], ls[1]);
}

/*public*/
inline double
LineSegment::distance(const Coordinate& p) const {
  return algorithm::Distance::pointToSegment(p, m_p0, m_p1);
}


inline const Coordinate&
LineSegment::operator[](std::size_t i) const {
  if(i == 0) {
    return m_p0;
  }
  assert(i == 1);
  return m_p1;
}


inline bool
LineSegment::isHorizontal() const {
  return std::abs(m_p0.y - m_p1.y) < geom::eps;
}

inline bool
LineSegment::isVertical() const {
  return std::abs(m_p0.x - m_p1.x) < geom::eps;
}


inline double LineSegment::angle() const {
  return m_angle;
}

inline double LineSegment::bearing() const {
  return m_bearing;
}

inline void LineSegment::midPoint(Coordinate& ret) const
{
  ret = .5 * (m_p0 + m_p1);
}

inline bool
operator==(const LineSegment& a, const LineSegment& b) {
  return a[0] == b[0] && a[1] == b[1];
}


} // namespace geos::geom
} // namespace geos

