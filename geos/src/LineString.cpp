/**********************************************************************
 *
 * GEOS - Geometry Engine Open Source
 * http://geos.osgeo.org
 *
 * Copyright (C) 2011 Sandro Santilli <strk@kbt.io>
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
 * Last port: geom/LineString.java r320 (JTS-1.12)
 *
 *********************************************************************

 Adapted for QOpenCPN (c) 2021 Jukka Sirkka

*/

#include "Coordinate.h"
#include "LineString.h"
#include "Distance.h"

#include <algorithm>
#include <typeinfo>
#include <memory>
#include <cassert>
#include <QDebug>

using namespace std;
using namespace geos::algorithm;

namespace geos {
namespace geom { // geos::geom

LineString::~LineString() {}

LineString::LineString(const LineString& ls)
  : m_points(ls.m_points)
  , m_segments(ls.m_segments)
  , m_length(ls.m_length)
  , m_cumulativeLengths(ls.m_cumulativeLengths)
{}

LineString::LineString(const CoordinateSequence& newCoords)
  : m_points(newCoords)
  , m_length(0)
{
  validateConstruction();

  m_cumulativeLengths << 0.;

  for (int i = 1; i < m_points.size(); i++) {
    auto seg = LineSegment(m_points[i - 1], m_points[i]);
    m_length += seg.length();
    m_segments << seg;
    m_cumulativeLengths << m_length;
  }
}

LineString::LineString()
  : m_points()
  , m_segments()
  , m_length(0.)
  , m_cumulativeLengths({0.})
{}

LineString LineString::reverse() const {
  if (isEmpty()) {
    return LineString(*this);
  }

  CoordinateSequence reversed(m_points.size());
  std::reverse_copy(std::begin(m_points), std::end(m_points), std::begin(reversed));

  return LineString(reversed);
}


/*private*/
void
LineString::validateConstruction() {
  if (m_points.size() == 1) {
    throw util::IllegalArgumentException("point array must contain 0 or >1 elements");
  }
}

bool LineString::isEmpty() const {
  return m_points.isEmpty();
}

bool LineString::isClosed() const {
  if (isEmpty()) {
    return false;
  }
  return m_points.last() == m_points.first();
}

bool LineString::isCoordinate(const Coordinate& pt) const {
  auto it = std::find_if(m_points.cbegin(), m_points.cend(), [pt] (const Coordinate& p) {
    return p == pt;
  });

  return it != m_points.cend();
}

int LineString::nextIndex(qreal pos) const {
  qDebug() << m_cumulativeLengths;
  for (int i = 0; i < m_cumulativeLengths.size(); ++i) {
    if (pos < m_cumulativeLengths[i]) {
      // qDebug() << i;
      return i;
    }
  }
  return m_cumulativeLengths.size();
}

qreal LineString::position(const Coordinate& inputPt, int endPoint) const {
  if (endPoint <= 1 || endPoint >= m_points.size() - 1) {
    return position(inputPt, begin(), end(), 0.);
  }
  SegmentIterator i_i = begin() + endPoint - 2;
  SegmentIterator i_f = begin() + endPoint + 1;
  return position(inputPt, i_i, i_f, length(endPoint - 2));
}

static double segmentNearestMeasure(const LineSegment& seg,
                                    const Coordinate& inputPt,
                                    qreal segmentStartMeasure) {
  // found new minimum, so compute location distance of point
  qreal projFactor = seg.projectionFactor(inputPt);
  if (projFactor <= 0.0) {
    // qDebug() << "negative projection factor" << projFactor;
    return segmentStartMeasure;
  }

  if (projFactor <= 1.0) {
    // qDebug() << "projection factor" << projFactor;
    return segmentStartMeasure + projFactor * seg.length();
  }
  // projFactor > 1.0
  // qDebug() << "large projection factor" << projFactor;
  return segmentStartMeasure + seg.length();
}


qreal LineString::position(const Coordinate& inputPt,
                           SegmentIterator start, SegmentIterator end,
                           qreal pos) const {

  qreal minDistance = numeric_limits<double>::max();

  double ptMeasure = 0.;
  double segmentStartMeasure = pos;

  for (SegmentIterator it = start; it != end; ++it) {

    const qreal segDistance = it->distance(inputPt);
    // qDebug() << "dist" << segDistance << it - start;
    double segMeasureToPt = segmentNearestMeasure(*it, inputPt, segmentStartMeasure);
    if (segDistance < minDistance) {
      ptMeasure = segMeasureToPt;
      minDistance = segDistance;
      // qDebug() << "minumum pos" << segMeasureToPt << it - start;
    }
    segmentStartMeasure += it->length();
  }

  if (ptMeasure < geom::eps) {
    const auto d0 = glm::distance(m_points.first(), inputPt);
    if (d0 < minDistance + geom::eps) {
      ptMeasure = - d0;
    }
  } else if (ptMeasure > length() - geom::eps) {
    const auto d1 = glm::distance(m_points.last(), inputPt);
    if (d1 < minDistance + geom::eps) {
       ptMeasure = length() + d1;
    }
  }

  return ptMeasure;
}


} // namespace geos::geom
} // namespace geos
