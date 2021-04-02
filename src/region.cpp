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
/* -*- coding: utf-8-unix -*-
 *
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

#include "region.h"
#include "region_p.h"
#include <QRectF>
#include "geoprojection.h"
#include <functional>

using namespace KV;


Region::Region(qreal x, qreal y, qreal w, qreal h) {
  Region tmp(QRectF(x, y, w, h));
  d = tmp.d;
}


Region::Region(const WGS84PointVector& cs, const GeoProjection* gp) {
  Region s;
  const int n = cs.size() / 2;
  for (int i = 0; i < n; i++) {
    // sw, ne
    s += QRectF(gp->fromWGS84(cs[2 * i]), gp->fromWGS84(cs[2 * i + 1]));
  }
  d = s.d;
}

WGS84PointVector Region::toWGS84(const GeoProjection *gp) const {
  WGS84PointVector ps;
  for (const QRectF& r: *this) {
    if (r.width() * r.height() < 1.) continue;
    ps << gp->toWGS84(r.topLeft()) << gp->toWGS84(r.bottomRight());
  }
  return ps;
}

Region Region::operator|(const Region &r) const {return united(r);}
Region Region::operator+(const Region &r) const {return united(r);}
Region Region::operator+(const QRectF &r) const {return united(r);}
Region Region::operator&(const Region &r) const {return intersected(r);}
Region Region::operator&(const QRectF &r) const {return intersected(r);}
Region Region::operator-(const Region &r) const {return subtracted(r);}
Region Region::operator^(const Region &r) const {return xored(r);}
Region& Region::operator|=(const Region &r) {return *this = *this | r;}

Region& Region::operator&=(const Region &r) {return *this = *this & r;}
Region& Region::operator&=(const QRectF &r) {return *this = *this & r;}
Region& Region::operator-=(const Region &r) {return *this = *this - r;}
Region& Region::operator^=(const Region &r) {return *this = *this ^ r;}

Region Region::translated(qreal dx, qreal dy) const {
  Region ret(*this);
  ret.translate(dx, dy);
  return ret;
}

qreal Region::area() const {
  return std::accumulate(d->rects.begin(), d->rects.end(), 0., [] (qreal s, const QRectF& r) {
    return s + r.width() * r.height();
  });
}


bool Region::intersects(const Region &region) const {
  if (!isValid() || !region.isValid()) return false;

  if (!boundingRect().intersects(region.boundingRect())) return false;

  if (rectCount() == 1 && region.rectCount() == 1) return true;

  for (const QRectF& myRect: d->rects) {
    for (const QRectF& otherRect: region.d->rects) {
      if (myRect.intersects(otherRect)) return true;
    }
  }

  return false;
}

struct UnionOps: public RegionOpInterface {
  void handleOverLap(RegionPrivate *dest,
                     BoxP r1, BoxP r1End,
                     BoxP r2, BoxP r2End,
                     qreal y1, qreal y2) const override {
    dest->unionOverLap(r1, r1End, r2, r2End, y1, y2);
  }
  void handleNonOverlap1(RegionPrivate *dest,
                         BoxP r1, BoxP r1End,
                         qreal y1, qreal y2) const override {
    dest->unionNonOverLap(r1, r1End, y1, y2);
  }
  void handleNonOverlap2(RegionPrivate *dest,
                         BoxP r1, BoxP r1End,
                         qreal y1, qreal y2) const override {
    dest->unionNonOverLap(r1, r1End, y1, y2);
  }
};


static void UnionRegion(const RegionPrivate *reg1, const RegionPrivate *reg2,
                        RegionPrivate &dest) {
  Q_ASSERT(!reg1->rects.isEmpty() && !reg2->rects.isEmpty());
  Q_ASSERT(!reg1->containsOuter(*reg2));
  Q_ASSERT(!reg2->containsOuter(*reg1));
  Q_ASSERT(!reg1->equals(reg2));

  UnionOps unionOps;
  dest.regionOp(reg1, reg2, &unionOps);

  dest.resetRects();
}




/*-
 *-----------------------------------------------------------------------
 * miSubtract --
 *      Subtract regS from regM and leave the result in regD.
 *      S stands for subtrahend, M for minuend and D for difference.
 *
 * Side Effects:
 *      regD is overwritten.
 *
 *-----------------------------------------------------------------------
 */

struct SubtractOps: public RegionOpInterface {
  void handleOverLap(RegionPrivate *dest,
                     BoxP r1, BoxP r1End,
                     BoxP r2, BoxP r2End,
                     qreal y1, qreal y2) const override {
    dest->subtractOverLap(r1, r1End, r2, r2End, y1, y2);
  }
  void handleNonOverlap1(RegionPrivate *dest,
                         BoxP r1, BoxP r1End,
                         qreal y1, qreal y2) const override {
    dest->subtractNonOverLap1(r1, r1End, y1, y2);
  }
  void handleNonOverlap2(RegionPrivate *dest,
                         BoxP r1, BoxP r1End,
                         qreal y1, qreal y2) const override {
    // noop
  }
};


static void SubtractRegion(const RegionPrivate *regM, const RegionPrivate *regS,
                           RegionPrivate &dest) {
  Q_ASSERT(!regM->rects.isEmpty() && !regS->rects.isEmpty());
  Q_ASSERT(regM->outerRect.intersects(regS->outerRect));
  Q_ASSERT(!regS->containsOuter(*regM));
  Q_ASSERT(!regM->equals(regS));

  SubtractOps subtractOps;
  dest.regionOp(regM, regS, &subtractOps);

  /*
   * Can't alter dest's outerRect before we call miRegionOp because
   * it might be one of the source regions and miRegionOp depends
   * on the outerRect of those regions being the unaltered. Besides, this
   * way there's no checking against rectangles that will be nuked
   * due to coalescing, so we have to examine fewer rectangles.
   */
  dest.resetRects();
}

static void XorRegion(const RegionPrivate *sra, const RegionPrivate *srb, RegionPrivate &dest) {
  Q_ASSERT(!sra->rects.isEmpty() && !srb->rects.isEmpty());
  Q_ASSERT(sra->outerRect.intersects(srb->outerRect));
  Q_ASSERT(!sra->equals(srb));

  RegionPrivate tra;
  RegionPrivate trb;

  if (!srb->containsOuter(*sra)) SubtractRegion(sra, srb, tra);
  if (!sra->containsOuter(*srb)) SubtractRegion(srb, sra, trb);

  Q_ASSERT(trb.rects.isEmpty() || !tra.containsOuter(trb));
  Q_ASSERT(tra.rects.isEmpty() || !trb.containsOuter(tra));

  if (tra.rects.isEmpty()) {
    dest.copy(trb);
  } else if (trb.rects.isEmpty()) {
    dest.copy(tra);
  } else {
    UnionRegion(&tra, &trb, dest);
  }
}

// END OF Region.c extract


Region::Region()
  : d(new RegionPrivate)
{}

Region::Region(const QRectF &r)
  : d(new RegionPrivate(r))
{}


Region::Region(const Region &r)
  : d(r.d)
{}

Region::~Region() {}


Region &Region::operator=(const Region &r) {
  d = r.d;
  return *this;
}

bool Region::isValid() const {
  return !isEmpty();
}

bool Region::isEmpty() const {
  return d->rects.isEmpty();
}

bool Region::contains(const QPointF &p) const {
  return d->contains(p.x(), p.y());
}

bool Region::contains(const QRectF &r) const {
  return d->contains(r.left(), r.top(), r.width(), r.height());
}

void Region::translate(qreal dx, qreal dy) {
  if ((dx == 0 && dy == 0) || !isValid()) return;
  d->translate(dx, dy);
}

Region Region::united(const Region &r) const {
  if (!isValid()) return r;
  if (!r.isValid()) return *this;
  if (d.data() == r.d.data()) return *this;

  if (d->containsOuter(*r.d.data())) {
    return *this;
  }

  if (r.d->containsOuter(*d.data())) {
    return r;
  }

  if (d->equals(r.d.data())) {
    return *this;
  }

  Region result;
  UnionRegion(d.data(), r.d.data(), *result.d.data());
  return result;
}

Region& Region::operator+=(const Region &r) {
  Region ret = united(r);
  d = ret.d;
  return *this;
}

Region Region::united(const QRectF &r) const {
  if (!isValid()) return r;

  if (r.isEmpty()) return *this;

  if (d->innerContains(r)) return *this;

  if (d->within(r)) return r;

  if (d->rects.size() == 1 && d->outerRect == r) return *this;


  Region result;
  const RegionPrivate rp(r);
  UnionRegion(d.data(), &rp, *result.d.data());
  return result;
}

Region& Region::operator+=(const QRectF &r) {
  Region ret = united(r);
  d = ret.d;
  return *this;
}

struct IntersectOps: public RegionOpInterface {
  void handleOverLap(RegionPrivate *dest,
                     BoxP r1, BoxP r1End,
                     BoxP r2, BoxP r2End,
                     qreal y1, qreal y2) const override {
    dest->intersectOverLap(r1, r1End, r2, r2End, y1, y2);
  }
  void handleNonOverlap1(RegionPrivate *dest,
                         BoxP r1, BoxP r1End,
                         qreal y1, qreal y2) const override {
    // noop
  }
  void handleNonOverlap2(RegionPrivate *dest,
                         BoxP r1, BoxP r1End,
                         qreal y1, qreal y2) const override {
    // noop
  }
};

Region Region::intersected(const Region &r) const {
  if (!isValid() || !r.isValid()
      || !d->outerRect.intersects(r.d->outerRect)) {
    return Region();
  }

  /* this is fully contained in r */
  if (r.d->containsOuter(*d.data())) return *this;

  /* r is fully contained in this */
  if (d->containsOuter(*r.d.data())) return r;

  Region result;
  IntersectOps intersectOps;
  result.d->regionOp(d, r.d, &intersectOps);

  /*
   * Can't alter dest's outerRect before we call miRegionOp because
   * it might be one of the source regions and miRegionOp depends
   * on the outerRect of those regions being the same. Besides, this
   * way there's no checking against rectangles that will be nuked
   * due to coalescing, so we have to examine fewer rectangles.
   */
  result.d->resetRects();
  return result;
}

Region Region::intersected(const QRectF &r) const {
  return intersected(Region(r));
}

Region Region::subtracted(const Region &r) const {
  if (!isValid() || !r.isValid()) return *this;
  if (r.d->containsOuter(*d.data())) return Region();

  if (!d->outerRect.intersects(r.d->outerRect)) return *this;

  if (d.data() == r.d.data() || d->equals(r.d.data())) return Region();


  Region result;
  SubtractRegion(d.data(), r.d.data(), *result.d.data());
  return result;
}

Region Region::xored(const Region &r) const {
  if (!isValid()) return r;
  if (!r.isValid()) return *this;
  if (!d->outerRect.intersects(r.d->outerRect)) return (*this + r);
  if (d.data() == r.d.data() || d->equals(r.d.data())) return Region();

  Region result;
  XorRegion(d.data(), r.d.data(), *result.d.data());
  return result;
}

QRectF Region::boundingRect() const noexcept {
  if (!isValid()) return QRectF();
  return d->outerRect;
}


Region::const_iterator Region::begin() const noexcept {
  return d->rects.begin();
}

Region::const_iterator Region::end() const noexcept {
  return d->rects.end();
}


int Region::rectCount() const noexcept {
  return d->rects.size();
}


bool Region::operator==(const Region &r) const {
  if (d.data() == r.d.data()) return true;
  return d->equals(r.d.data());
}

bool Region::intersects(const QRectF &rect) const {
  if (!isValid() || rect.isEmpty()) return false;

  if (!rect.intersects(d->outerRect)) return false;

  if (d->rects.size() == 1) return true;

  for (const QRectF &r: d->rects) {
    if (r.intersects(rect)) return true;
  }

  return false;
}
