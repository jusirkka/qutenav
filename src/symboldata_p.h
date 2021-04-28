/* -*- coding: utf-8-unix -*-
 *
 * File: src/symboldata_p.h
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
#include <QPoint>
#include "types.h"
#include "s57object.h"

struct SymbolDataPrivate: public QSharedData {

  using ColorVector = QVector<S52::Color>;

  inline SymbolDataPrivate()
    : size()
  {}

  inline SymbolDataPrivate(const SymbolDataPrivate& d)
    : QSharedData(d)
    , offset(d.offset)
    , size(d.size)
    , advance(d.advance)
    , elements(d.elements)
    , colors(d.colors) {}

  inline void computeAdvance(qreal mnd, bool st) {
    // check if pivot is outside of the rect(offset, w, h) and enlarge
    // note: inverted y-axis
    const QPointF origin(offset.x(), offset.y() - size.height());
    const QRectF rtest(origin, size);
    auto r1 = rtest.united(QRectF(QPointF(0, 0), QSizeF(.1, .1)));
    const qreal x0 = r1.width() + mnd;
    const qreal y0 = r1.height() + mnd;
    const qreal x1 = st ? .5 * x0 : 0.;
    advance = PatternMMAdvance(x0, y0, x1);
  }

  QPointF offset;
  QSizeF size;
  PatternMMAdvance advance;
  S57::ElementDataVector elements;
  ColorVector colors;
};
