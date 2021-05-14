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

#pragma once

#include <QPointF>
#include <QSharedDataPointer>
#include <QMap>
#include "types.h"

class QRectF;
class QPolygonF;

class GeoProjection;

namespace KV {

class RegionPrivate;

class Region {
public:


  Region();
  Region(qreal x, qreal y, qreal w, qreal h);
  Region(const QRectF& r);
  Region(const Region& region);

  Region(const WGS84PointVector& cs, const GeoProjection* gp);

  ~Region();

  Region &operator=(const Region&);
  inline Region &operator=(Region&& other) noexcept {
    qSwap(d, other.d); return *this;
  }

  WGS84PointVector toWGS84(const GeoProjection* gp) const;

  bool isValid() const;
  bool isEmpty() const;

  using const_iterator = QVector<QRectF>::const_iterator;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  const_iterator begin()  const noexcept;
  const_iterator end()    const noexcept;
  const_reverse_iterator rbegin()  const noexcept {return const_reverse_iterator(end());}
  const_reverse_iterator rend()    const noexcept {return const_reverse_iterator(begin());}

  bool contains(const QPointF& p) const;
  bool contains(const QRectF& r) const;

  void translate(qreal dx, qreal dy);
  inline void translate(const QPointF &p) {translate(p.x(), p.y());}

  Region translated(qreal dx, qreal dy) const;
  inline Region translated(const QPointF &p) const {return translated(p.x(), p.y());}

  Region united(const Region &r) const;
  Region united(const QRectF &r) const;
  Region intersected(const Region &r) const;
  Region intersected(const QRectF &r) const;
  Region subtracted(const Region &r) const;
  Region xored(const Region &r) const;

  bool intersects(const Region &r) const;
  bool intersects(const QRectF &r) const;

  QRectF boundingRect() const noexcept;

  int rectCount() const noexcept;
  QVector<QRectF> rects() const;

  Region operator|(const Region &r) const;
  Region operator+(const Region &r) const;
  Region operator+(const QRectF &r) const;
  Region operator&(const Region &r) const;
  Region operator&(const QRectF &r) const;
  Region operator-(const Region &r) const;
  Region operator^(const Region &r) const;

  Region& operator|=(const Region &r);
  Region& operator+=(const Region &r);
  Region& operator+=(const QRectF &r);
  Region& operator&=(const Region &r);
  Region& operator&=(const QRectF &r);
  Region& operator-=(const Region &r);
  Region& operator^=(const Region &r);

  bool operator==(const Region &r) const;
  inline bool operator!=(const Region &r) const {return !(operator==(r));}

  qreal area() const;

private:

    QSharedDataPointer<RegionPrivate> d;

};

using RegionMap = QMap<quint32, Region>;

} // namespace KV
