/**********************************************************************
 *
 * GEOS - Geometry Engine Open Source
 * http://geos.osgeo.org
 *
 * Copyright (C) 2011 Sandro Santilli <strk@kbt.io>
 * Copyright (C) 2001-2002 Vivid Solutions Inc.
 * Copyright (C) 2005 2006 Refractions Research Inc.
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU Lesser General Public Licence as published
 * by the Free Software Foundation.
 * See the COPYING file for more information.
 *
 **********************************************************************
 *
 * Last port: geom/LineString.java r320 (JTS-1.12)
 *
 *********************************************************************

 Adapted for QuteNav (c) 2021 Jukka Sirkka

*/

#pragma once

#include <vector>
#include <memory> // for unique_ptr

#include "Coordinate.h"
#include "LineSegment.h"

namespace geos {
namespace geom { // geos::geom


/**
 *  Models an OGC-style <code>LineString</code>.
 *
 *  A LineString consists of a sequence of two or more vertices,
 *  along with all points along the linearly-interpolated curves
 *  (line segments) between each
 *  pair of consecutive vertices.
 *  Consecutive vertices may be equal.
 *  The line segments in the line may intersect each other (in other words,
 *  the linestring may "curl back" in itself and self-intersect).
 *  Linestrings with exactly two identical points are invalid.
 *
 *  A linestring must have either 0 or 2 or more points.
 *  If these conditions are not met, the constructors throw
 *  an {@link util::IllegalArgumentException}.
 */
class LineString {

public:

  ~LineString();

  /**
     * \brief
     * Creates and returns a full copy of this {@link LineString} object
     * (including all coordinates contained by it)
     *
     * @return A clone of this instance
     */
  std::unique_ptr<LineString> clone() const;

  /// Returns a read-only pointer to internal CoordinateSequence
  const CoordinateSequence& coordinates() const;

  Coordinate coordinate(size_t n) const;


  bool isEmpty() const;

  int size() const {return m_points.size();}

  bool isClosed() const;


  bool isCoordinate(const Coordinate& pt) const;


  /**
     * Creates a LineString whose coordinates are in the reverse
     * order of this object's
     *
     * @return a LineString with coordinates in the reverse order
     */
  LineString reverse() const;

  LineString(const LineString& ls);
  LineString(const CoordinateSequence& pts);
  LineString();


  int nextIndex(qreal pos) const;

  LineSegment segment(int index) const {
    if (index < 0) return m_segments.first();
    if (index >= m_segments.size()) return m_segments.last();
    return m_segments[index];
  }

  using SegmentVector = QVector<LineSegment>;

  using SegmentIterator = SegmentVector::const_iterator;

  SegmentIterator begin() const noexcept {return m_segments.cbegin();}
  SegmentIterator end() const noexcept {return m_segments.cend();}

  double length() const {return m_length;}
  double length(int segmentEndIndex) const {
    if (segmentEndIndex >= m_cumulativeLengths.size()) {
      return m_length;
    }
    if (segmentEndIndex < 0) {
      return 0.;
    }
    return m_cumulativeLengths[segmentEndIndex];
  }

  /**
     * Find the nearest location along a linear Geometry to a given point.
     *
     * @param inputPt the coordinate to locate
     * @return the location of the nearest point
     */
  qreal position(const geom::Coordinate& inputPt, int curr) const;


private:

  qreal position(const geom::Coordinate& inputPt,
                 SegmentIterator start, SegmentIterator end,
                 qreal pos) const;

  CoordinateSequence m_points;
  SegmentVector m_segments;
  double m_length;

  void validateConstruction();

  using LengthVector = QVector<qreal>;

  LengthVector m_cumulativeLengths;

};

} // namespace geos::geom
} // namespace geos

