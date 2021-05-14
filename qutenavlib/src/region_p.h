/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

// START OF Region.c extract
/* $XConsortium: Region.c /main/30 1996/10/22 14:21:24 kaleb $ */
/************************************************************************

Copyright (c) 1987, 1988  X Consortium

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of the X Consortium shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from the X Consortium.


Copyright 1987, 1988 by Digital Equipment Corporation, Maynard, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of Digital not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

************************************************************************/
/*
 * The functions in this file implement the Region abstraction, similar to one
 * used in the X11 sample server. A Region is simply an area, as the name
 * implies, and is implemented as a "y-x-banded" array of rectangles. To
 * explain: Each Region is made up of a certain number of rectangles sorted
 * by y coordinate first, and then by x coordinate.
 *
 * Furthermore, the rectangles are banded such that every rectangle with a
 * given upper-left y coordinate (y1) will have the same lower-right y
 * coordinate (y2) and vice versa. If a rectangle has scanlines in a band, it
 * will span the entire vertical distance of the band. This means that some
 * areas that could be merged into a taller rectangle will be represented as
 * several shorter rectangles to account for shorter rectangles to its left
 * or right but within its "vertical scope".
 *
 * An added constraint on the rectangles is that they must cover as much
 * horizontal area as possible. E.g. no two rectangles in a band are allowed
 * to touch.
 *
 * Whenever possible, bands will be merged together to cover a greater vertical
 * distance (and thus reduce the number of rectangles). Two bands can be merged
 * only if the bottom of one touches the top of the other and they have
 * rectangles in the same places (of the same width, of course). This maintains
 * the y-x-banding that's so nice to have...
 */
/* $XFree86: xc/lib/X11/Region.c,v 1.1.1.2.2.2 1998/10/04 15:22:50 hohndel Exp $ */


/* -*- coding: utf-8-unix -*-
 *
 * region_p.h
 *
 * Created: 2021-03-14 2021 by Jukka Sirkka
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
#pragma once


#include <QSharedData>
#include <QRectF>

namespace KV {

struct RegionOpInterface;

struct RegionPrivate: public QSharedData {

  using RectVector = QVector<QRectF>;

  // tolerance when comparing rectangles
  static const inline qreal tol = 1.e-4;

  int innerArea;
  RectVector rects;
  QRectF outerRect;
  QRectF innerRect;

  RegionPrivate() : innerArea(-1) {}

  RegionPrivate(const QRectF &r)
    : innerArea(r.width() * r.height())
    , rects({r})
    , outerRect(r)
    , innerRect(r)
  {
    Q_ASSERT(!r.isEmpty());
  }

  /*
   * Returns \c true if r is guaranteed to be fully contained in this region.
   * A false return value does not guarantee the opposite.
   */
  bool containsOuter(const RegionPrivate &r) const {
    return innerContains(r.outerRect);
  }

  bool innerContains(const QRectF &r2) const {
    return innerRect.contains(r2);
  }

  /*
   * Returns \c true if this region is guaranteed to be fully contained in r.
   */
  bool within(const QRectF &r1) const {
    return r1.contains(outerRect);
  }

  void updateInnerRect(const QRectF &rect) {
    const qreal area = rect.width() * rect.height();
    if (area > innerArea) {
      innerArea = area;
      innerRect = rect;
    }
  }

  bool equals(const RegionPrivate* other) const;

  bool contains(qreal rx, qreal ry, qreal rwidth, qreal rheight) const;
  bool contains(qreal x, qreal y) const;

  void copy(const RegionPrivate& other);

  void resetRects();
  void translate(qreal x, qreal y);

  int coalesce(int prevStart, int curStart);

  using BoxP = RectVector::const_iterator;


  void regionOp(const RegionPrivate *reg1, const RegionPrivate *reg2,
                RegionOpInterface* operation);

  void intersectOverLap(BoxP r1, BoxP r1End,
                        BoxP r2, BoxP r2End,
                        qreal y1, qreal y2);

  void unionNonOverLap(BoxP r1, BoxP r1End,
                       qreal y1, qreal y2);

  void unionOverLap(BoxP r1, BoxP r1End,
                    BoxP r2, BoxP r2End,
                    qreal y1, qreal y2);

  void subtractNonOverLap1(BoxP r1, BoxP r1End,
                           qreal y1, qreal y2);

  void subtractOverLap(BoxP r1, BoxP r1End,
                       BoxP r2, BoxP r2End,
                       qreal y1, qreal y2);

};


using BoxP = RegionPrivate::BoxP;

struct RegionOpInterface {
  virtual void handleOverLap(RegionPrivate* dest,
                             BoxP r1, BoxP r1End,
                             BoxP r2, BoxP r2End,
                             qreal y1, qreal y2) const = 0;
  virtual void handleNonOverlap1(RegionPrivate* dest,
                                 BoxP r1, BoxP r1End,
                                 qreal y1, qreal y2) const = 0;
  virtual void handleNonOverlap2(RegionPrivate* dest,
                                 BoxP r1, BoxP r1End,
                                 qreal y1, qreal y2) const = 0;
};



bool RegionPrivate::equals(const RegionPrivate *r) const {
  if (rects.size() != r->rects.size()) return false;
  if (rects.isEmpty()) return true;
  if (outerRect != r->outerRect) return false;
  if (rects.size() == 1) return true; // equality tested in previous if-statement

  for (int i = 0; i < rects.size(); i++) {
    const QRectF& r1 = rects[i];
    const QRectF& r2 = r->rects[i];
    if (r1 != r2) return false;
  }

  return true;
}

bool RegionPrivate::contains(qreal x, qreal y) const {
  if (rects.isEmpty()) return false;
  if (!outerRect.contains(x, y)) return false;
  if (innerRect.contains(x, y)) return true;

  for (const QRectF& r: rects) {
    if (r.contains(x, y)) return true;
  }

  return false;
}

bool RegionPrivate::contains(qreal rx, qreal ry, qreal rwidth, qreal rheight) const {

  const QRectF rect(rx, ry, rwidth, rheight);

  if (rects.isEmpty() || !outerRect.intersects(rect)) {
    return false;
  }

  bool partOut = false;
  bool partIn = false;

  /* can stop when both partOut and partIn are true, or we reach rect.bottom */
  for (BoxP pbox = rects.begin(); pbox != rects.end(); ++pbox) {

    if (pbox->bottom() < ry) continue;

    if (pbox->top() > ry) {
      partOut = true;
      if (partIn || pbox->top() > rect.bottom()) break;
      ry = pbox->top();
    }

    if (pbox->right() < rx) continue; /* not far enough over yet */

    if (pbox->left() > rx) {
      partOut = true;      /* missed part of rectangle to left */
      if (partIn) break;
    }

    if (pbox->left() <= rect.right()) {
      partIn = true;      /* definitely overlap */
      if (partOut) break;
    }

    if (pbox->right() >= rect.right()) {
      ry = pbox->bottom();     /* finished with this band */
      if (ry > rect.bottom()) break;
      rx = rect.left();  /* reset x out to left again */
    } else {
      /*
       * Because boxes in a band are maximal width, if the first box
       * to overlap the rectangle doesn't completely cover it in that
       * band, the rectangle must be partially out, since some of it
       * will be uncovered in that band. partIn will have been set true
       * by now...
       */
      break;
    }
  }
  return partIn;
}


void RegionPrivate::copy(const RegionPrivate &d) {
  innerArea = d.innerArea;
  rects = d.rects;
  outerRect = d.outerRect;
  innerRect = d.innerRect;
}

void RegionPrivate::resetRects() {

  // remove zero area rects
  RectVector::iterator it = rects.begin();
  while (it != rects.end()) {
    if (it->height() < tol || it->width() < tol) {
      it = rects.erase(it);
    } else {
      ++it;
    }
  }

  innerRect.setCoords(0, 0, 0, 0);
  innerArea = -1;
  if (rects.isEmpty()) {
    outerRect.setCoords(0, 0, 0, 0);
    return;
  }

  const QRectF& first = rects.first();
  const QRectF& last = rects.last();

  /*
   * Since pBox is the first rectangle in the region, it must have the
   * smallest y1 and since pBoxEnd is the last rectangle in the region,
   * it must have the largest y2, because of banding. Initialize x1 and
   * x2 from  pBox and pBoxEnd, resp., as good things to initialize them
   * to...
   */
  outerRect.setTop(first.top());
  outerRect.setBottom(last.bottom());

  outerRect.setLeft(first.left());
  outerRect.setRight(last.right());

  Q_ASSERT(outerRect.top() < outerRect.bottom());
  for (const QRectF& r: rects) {
    Q_ASSERT(r.left() < r.right() && r.top() < r.bottom());
    if (r.left() < outerRect.left()) {
        outerRect.setLeft(r.left());
    }
    if (r.right() > outerRect.right()) {
      outerRect.setRight(r.right());
    }
    updateInnerRect(r);
  }

  Q_ASSERT(outerRect.left() < outerRect.right());
}

void RegionPrivate::translate(qreal x, qreal y) {
  if (rects.isEmpty()) return;

  for (QRectF& r: rects) {
    r.translate(x, y);
  }

  outerRect.translate(x, y);
  innerRect.translate(x, y);
}

/*-
 *-----------------------------------------------------------------------
 * miCoalesce --
 *      Attempt to merge the boxes in the current band with those in the
 *      previous one. Used only by miRegionOp.
 *
 * Results:
 *      The new index for the previous band.
 *
 * Side Effects:
 *      If coalescing takes place:
 *          - rectangles in the previous band will have their y2 fields
 *            altered.
 *          - dest.numRects will be decreased.
 *
 *-----------------------------------------------------------------------
 */
int RegionPrivate::coalesce(int prevStart, int currStart) {

  if (currStart == rects.size()) {
    return prevStart;
  }

  const qreal bandY1 = rects[currStart].top(); //  Y1 coordinate for current band
  const int regEnd = rects.size();  // End of region

  int ret = currStart; // the new prevStart

  /*
   * Figure out how many rectangles are in the current band. Have to do
   * this because multiple bands could have been added in miRegionOp
   * at the end when one region has been exhausted.
   */
  // This is the number of rects in the first added band
  int currNumRects = 1;
  int curr = currStart + 1;
  for (; curr < regEnd && std::abs(rects[curr].top() - bandY1) < tol; ++curr) {
    ++currNumRects;
  }

  if (curr < regEnd) {
    /*
     * If more than one band was added, we have to find the start
     * of the last band added so the next coalescing job can start
     * at the right place...
     */
    int last = regEnd - 1;
    while (std::abs(rects[last - 1].top() - rects[last].top()) < tol) --last;
    ret = last;
  }


  const int prevNumRects = currStart - prevStart; // Number of rectangles in previous band
  if (currNumRects == prevNumRects) {
    /*
     * The bands may only be coalesced if the bottom of the previous
     * matches the top scanline of the current.
     */
    if (std::abs(rects[prevStart].bottom() - bandY1) < tol) {
      /*
       * Make sure the bands have boxes in the same places. This
       * assumes that boxes have been added in such a way that they
       * cover the most area possible. I.e. two boxes in a band must
       * have some horizontal space between them.
       */
      for (int i = 0; i < currNumRects; i++) {
        const QRectF& prev = rects[prevStart + i];
        const QRectF& curr = rects[currStart + i];
        if (std::abs(prev.left() - curr.left()) >= tol || std::abs(prev.right() - curr.right()) >= tol) {
          // The bands don't line up so they can't be coalesced.
          return ret;
        }
      }

      /*
       * The bands may be merged, so set the bottom y of each box
       * in the previous band to that of the corresponding box in
       * the current band.
       */
      // rs is the coalesced rects
      RectVector rs = rects.mid(0, currStart);
      for (int i = 0; i < currNumRects; i++) {
        rs[prevStart + i].setBottom(rects[currStart + i].bottom());
      }

      if (currStart + currNumRects < regEnd) {
        // If more than one band was added to the region, copy the
        // other bands up. The assumption here is that the other bands
        // came from the same region as the current one and no further
        // coalescing can be done on them since it's all been done
        // already...
        rs.append(rects.mid(currStart + currNumRects));
      }

      rects = rs;
    }
  }

  return ret;
}

/*-
 *-----------------------------------------------------------------------
 * miRegionOp --
 *      Apply an operation to two regions. Called by miUnion, miInverse,
 *      miSubtract, miIntersect...
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      The new region is overwritten.
 *
 * Notes:
 *      The idea behind this function is to view the two regions as sets.
 *      Together they cover a rectangle of area that this function divides
 *      into horizontal bands where points are covered only by one region
 *      or by both. For the first case, the nonOverlapFunc is called with
 *      each the band and the band's upper and lower extents. For the
 *      second, the overlapFunc is called to process the entire band. It
 *      is responsible for clipping the rectangles in the band, though
 *      this function provides the boundaries.
 *      At the end of each band, the new region is coalesced, if possible,
 *      to reduce the number of rectangles in the region.
 *
 *-----------------------------------------------------------------------
 */
void RegionPrivate::regionOp(const RegionPrivate *reg1, const RegionPrivate *reg2,
                             RegionOpInterface* operation) {

  /*
   * Initialize ybot.
   * In the upcoming loop, ybot and ytop serve different functions depending
   * on whether the band being handled is an overlapping or non-overlapping
   * band.
   *  In the case of a non-overlapping band (only one of the regions
   * has points in the band), ybot is the bottom of the most recent
   * intersection and thus clips the top of the rectangles in that band.
   * ytop is the top of the next intersection between the two regions and
   * serves to clip the bottom of the rectangles in the current band.
   *  For an overlapping band (where the two regions intersect), ytop clips
   * the top of the rectangles of both regions and ybot clips the bottoms.
   */

  qreal ybot; // Bottom of intersection
  if (reg1->outerRect.top() < reg2->outerRect.top()) {
    ybot = reg1->outerRect.top();
  } else {
    ybot = reg2->outerRect.top();
  }

  /*
   * prevBand serves to mark the start of the previous band so rectangles
   * can be coalesced into larger rectangles. See for miCoalesce, above.
   * In the beginning, there is no previous band, so prevBand == curBand
   * (curBand is set later on, of course, but the first band will always
   * start at index 0). prevBand and curBand must be indices because of
   * the possible expansion, and resultant moving, of the new region's
   * array of rectangles.
   */
  int prevBand = 0;

  BoxP r1 = reg1->rects.begin(); // Pointer into 1st region
  BoxP r2 = reg2->rects.begin(); // Pointer into 2nd region
  const BoxP r1End = reg1->rects.end(); // End of 1st region
  const BoxP r2End = reg2->rects.end(); // End of 2nd region
  BoxP r1BandEnd;  // End of current band in r1
  BoxP r2BandEnd;  // End of current band in r2





  do {


    /*
     * This algorithm proceeds one source-band (as opposed to a
     * destination band, which is determined by where the two regions
     * intersect) at a time. r1BandEnd and r2BandEnd serve to mark the
     * rectangle after the last one in the current band for their
     * respective regions.
     */
    r1BandEnd = r1;
    while (r1BandEnd != r1End && std::abs(r1BandEnd->top() - r1->top()) < tol) ++r1BandEnd;

    r2BandEnd = r2;
    while (r2BandEnd != r2End && std::abs(r2BandEnd->top() - r2->top()) < tol) ++r2BandEnd;

    /*
     * First handle the band that doesn't intersect, if any.
     *
     * Note that attention is restricted to one band in the
     * non-intersecting region at once, so if a region has n
     * bands between the current position and the next place it overlaps
     * the other, this entire loop will be passed through n times.
     */
    qreal ytop; // Top of intersection
    int curBand = rects.size(); // Index of start of current band after applying non-o func

    if (std::abs(r1->top() - r2->top()) < tol) {
      ytop = .5 * (r1->top() + r2->top());
    } else if (r1->top() < r2->top()) {
      const qreal top = qMax(r1->top(), ybot);
      const qreal bot = qMin(r1->bottom(), r2->top());

      if (bot > top) {
        operation->handleNonOverlap1(this, r1, r1BandEnd, top, bot);
      }
      ytop = r2->top();
    } else {
      const qreal top = qMax(r2->top(), ybot);
      const qreal bot = qMin(r2->bottom(), r1->top());

      if (bot > top) {
        operation->handleNonOverlap2(this, r2, r2BandEnd, top, bot);
      }
      ytop = r1->top();
    }

    /*
     * If any rectangles got added to the region, try and coalesce them
     * with rectangles from the previous band.
     */
    prevBand = coalesce(prevBand, curBand);

    /*
     * Now see if we've hit an intersecting band. The two bands only
     * intersect if ybot >= ytop
     */
    ybot = qMin(r1->bottom(), r2->bottom());
    curBand = rects.size(); // Index of start of current band after applying olap func
    if (ybot > ytop + tol) {
      operation->handleOverLap(this, r1, r1BandEnd, r2, r2BandEnd, ytop, ybot);
    }

    prevBand = coalesce(prevBand, curBand);

    /*
     * If we've finished with a band (y2 == ybot) we skip forward
     * in the region to the next band.
     */
    if (std::abs(r1->bottom() - ybot) < tol) r1 = r1BandEnd;
    if (std::abs(r2->bottom() - ybot) < tol) r2 = r2BandEnd;

  } while (r1 != r1End && r2 != r2End);

  /*
   * Deal with whichever region still has rectangles left.
   */
  const int curBand = rects.size(); // Index of start of current band after applying non-o func
  if (r1 != r1End) {
    do {
      r1BandEnd = r1;
      while (r1BandEnd < r1End && std::abs(r1BandEnd->top() - r1->top()) < tol) ++r1BandEnd;
      operation->handleNonOverlap1(this, r1, r1BandEnd, qMax(r1->top(), ybot), r1->bottom());
      r1 = r1BandEnd;
    } while (r1 != r1End);
  } else if (r2 != r2End) {
    do {
      r2BandEnd = r2;
      while (r2BandEnd < r2End && std::abs(r2BandEnd->top() - r2->top()) < tol) ++r2BandEnd;
      operation->handleNonOverlap2(this, r2, r2BandEnd, qMax(r2->top(), ybot), r2->bottom());
      r2 = r2BandEnd;
    } while (r2 != r2End);
  }

  coalesce(prevBand, curBand);

}


/*-
 *-----------------------------------------------------------------------
 * miIntersectO --
 *      Handle an overlapping band for miIntersect.
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      Rectangles may be added to the region.
 *
 *-----------------------------------------------------------------------
 */
void RegionPrivate::intersectOverLap(BoxP r1, BoxP r1End,
                                     BoxP r2, BoxP r2End,
                                     qreal y1, qreal y2) {


  while (r1 != r1End && r2 != r2End) {
    const auto L = qMax(r1->left(), r2->left());
    const auto R = qMin(r1->right(), r2->right());

    /*
     * If there's any overlap between the two rectangles, add that
     * overlap to the new region.
     * There's no need to check for subsumption because the only way
     * such a need could arise is if some region has two rectangles
     * right next to each other. Since that should never happen...
     */
    if (L < R) {
      rects << QRectF(L, y1, R - L, y2 - y1);
    }

    /*
     * Need to advance the pointers. Shift the one that extends
     * to the right the least, since the other still has a chance to
     * overlap with that region's next rectangle, if you see what I mean.
     */
    if (std::abs(r1->right() - r2->right()) < tol) {
      ++r1;
      ++r2;
    } else if (r1->right() < r2->right()) {
      ++r1;
    } else {
      ++r2;
    }
  }
}


/*-
 *-----------------------------------------------------------------------
 * miUnionNonO --
 *      Handle a non-overlapping band for the union operation. Just
 *      Adds the rectangles into the region. Doesn't have to check for
 *      subsumption or anything.
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      dest.numRects is incremented and the final rectangles overwritten
 *      with the rectangles we're passed.
 *
 *-----------------------------------------------------------------------
 */

void RegionPrivate::unionNonOverLap(BoxP r, BoxP rEnd, qreal y1, qreal y2) {
  while (r != rEnd) {
    Q_ASSERT(r->left() < r->right());
    rects << QRectF(r->left(), y1, r->width(), y2 - y1);
    ++r;
  }
}

/*-
 *-----------------------------------------------------------------------
 * miUnionO --
 *      Handle an overlapping band for the union operation. Picks the
 *      left-most rectangle each time and merges it into the region.
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      Rectangles are overwritten in dest.rects and dest.numRects will
 *      be changed.
 *
 *-----------------------------------------------------------------------
 */

void RegionPrivate::unionOverLap(BoxP r1, BoxP r1End,
                                 BoxP r2, BoxP r2End,
                                 qreal y1, qreal y2) {

  auto merge = [this, y1, y2] (BoxP r) {

    QRectF& last = rects.isEmpty() ? innerRect : rects.last();
    if (!rects.isEmpty() && std::abs(last.top() - y1) < tol &&
        std::abs(last.bottom() - y2) < tol && last.right() >= r->left() - tol) {
      if (last.right() < r->right()) {
        last.setRight(r->right());
        updateInnerRect(last);
        Q_ASSERT(last.left() < last.right());
      }
    } else {
      auto box = QRectF(r->left(), y1, r->width(), y2 - y1);
      updateInnerRect(box);
      rects << box;
    }
  };

  Q_ASSERT(y1 < y2);
  while (r1 != r1End && r2 != r2End) {
    if (r1->left() < r2->left()) {
      merge(r1);
      ++r1;
    } else {
      merge(r2);
      ++r2;
    }
  }

  if (r1 != r1End) {
    do {
      merge(r1);
      ++r1;
    } while (r1 != r1End);
  } else {
    while (r2 != r2End) {
      merge(r2);
      ++r2;
    }
  }
}

/*-
 *-----------------------------------------------------------------------
 * miSubtractNonO --
 *      Deal with non-overlapping band for subtraction. Any parts from
 *      region 2 we discard. Anything from region 1 we add to the region.
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      dest may be affected.
 *
 *-----------------------------------------------------------------------
 */

void RegionPrivate::subtractNonOverLap1(BoxP r, BoxP rEnd, qreal y1, qreal y2) {

  Q_ASSERT(y1 < y2);

  while (r != rEnd) {
    rects << QRectF(r->left(), y1, r->width(), y2 - y1);
    ++r;
  }
}

/*-
 *-----------------------------------------------------------------------
 * miSubtractO --
 *      Overlapping band subtraction. x1 is the left-most point not yet
 *      checked.
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      dest may have rectangles added to it.
 *
 *-----------------------------------------------------------------------
 */

void RegionPrivate::subtractOverLap(BoxP r1, BoxP r1End,
                                    BoxP r2, BoxP r2End,
                                    qreal y1, qreal y2) {
  qreal x1 = r1->left();

  Q_ASSERT(y1 < y2);

  while (r1 != r1End && r2 != r2End) {
    if (r2->right() < x1) {
      /*
       * Subtrahend missed the boat: go to next subtrahend.
       */
      ++r2;
    } else if (r2->left() < x1 + tol) {
      /*
       * Subtrahend precedes minuend: nuke left edge of minuend.
       */
      x1 = r2->right();
      if (x1 + tol > r1->right()) {
        /*
         * Minuend completely covered: advance to next minuend and
         * reset left fence to edge of new minuend.
         */
        ++r1;
        if (r1 != r1End) {
          x1 = r1->left();
        }
      } else {
        // Subtrahend now used up since it doesn't extend beyond minuend
        ++r2;
      }
    } else if (r2->left() < r1->right() + tol) {
      /*
       * Left part of subtrahend covers part of minuend: add uncovered
       * part of minuend to region and skip to next subtrahend.
       */
      rects << QRectF(x1, y1, r2->left() - x1, y2 - y1);

      x1 = r2->right();
      if (x1 + tol > r1->right()) {
        /*
         * Minuend used up: advance to new...
         */
        ++r1;
        if (r1 != r1End) {
          x1 = r1->left();
        }
      } else {
        // Subtrahend used up
        ++r2;
      }
    } else {
      /*
       * Minuend used up: add any remaining piece before advancing.
       */
      if (r1->right() + tol > x1) {
        rects << QRectF(x1, y1, r1->right() - x1, y2 - y1);
      }
      ++r1;
      if (r1 != r1End) {
        x1 = r1->left();
      }
    }
  }

  /*
   * Add remaining minuend rectangles to region.
   */
  while (r1 != r1End) {
    rects << QRectF(x1, y1, r1->right() - x1, y2 - y1);
    ++r1;
    if (r1 != r1End) {
      x1 = r1->left();
    }
  }
}




} // namespace KV
