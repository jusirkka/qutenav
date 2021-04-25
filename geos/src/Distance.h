/**********************************************************************
 *
 * GEOS - Geometry Engine Open Source
 * http://geos.osgeo.org
 *
 * Copyright (C) 2018 Paul Ramsey <pramsey@cleverlephant.ca>
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU Lesser General Public Licence as published
 * by the Free Software Foundation.
 * See the COPYING file for more information.
 *
 **********************************************************************
 *
 * Last port: algorithm/Distance.java @ 2017-09-04
 *
 *********************************************************************

 Adapted for QOpenCPN (c) 2021 Jukka Sirkka

*/

#pragma once

#include "Coordinate.h"

namespace geos {
namespace algorithm { // geos::algorithm

/** \brief
 * Functions to compute distance between basic geometric structures.
 *
 * @author Martin Davis
 *
 */
class Distance {
public:

  /**
     * Computes the distance from a line segment AB to a line segment CD
     *
     * Note: NON-ROBUST!
     *
     * @param A
     *          a point of one line
     * @param B
     *          the second point of (must be different to A)
     * @param C
     *          one point of the line
     * @param D
     *          another point of the line (must be different to A)
     */
  // formerly distanceLineLine
  static double segmentToSegment(const geom::Coordinate& A,
                                 const geom::Coordinate& B,
                                 const geom::Coordinate& C,
                                 const geom::Coordinate& D);

  /**
    * Computes the distance from a point p to a line segment AB
    *
    * Note: NON-ROBUST!
    *
    * @param p
    *          the point to compute the distance for
    * @param A
    *          one point of the line
    * @param B
    *          another point of the line (must be different to A)
    * @return the distance from p to line segment AB
    */
  // formerly distancePointLine
  static double pointToSegment(const geom::Coordinate& p,
                               const geom::Coordinate& A,
                               const geom::Coordinate& B);

  /**
    * Computes the perpendicular distance from a point p to the (infinite) line
    * containing the points AB
    *
    * @param p
    *          the point to compute the distance for
    * @param A
    *          one point of the line
    * @param B
    *          another point of the line (must be different to A)
    * @return the distance from p to line AB
    */
  // formerly distancePointLinePerpendicular
  static double pointToLinePerpendicular(const geom::Coordinate& p,
                                         const geom::Coordinate& A,
                                         const geom::Coordinate& B);

};

} // namespace geos::algorithm
} // namespace geos

