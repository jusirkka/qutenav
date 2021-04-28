/* -*- coding: utf-8-unix -*-
 *
 * File: src/symboldata.cpp
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
#include "symboldata.h"
#include "symboldata_p.h"

SymbolData::SymbolData()
  : d(new SymbolDataPrivate)
{}

SymbolData::SymbolData(const SymbolData& s)
  : d(s.d)
{}

SymbolData& SymbolData::operator=(const SymbolData& s) {
  d = s.d;
  return *this;
}

SymbolData::SymbolData(const QPointF& off,
                       const QSizeF& size,
                       qreal mnd,
                       bool st,
                       const S57::ElementData& elem)
  : d(new SymbolDataPrivate) {

  d->offset = off;
  d->size = size;
  d->elements.append(elem);
  d->computeAdvance(mnd, st);
}

SymbolData::SymbolData(const QPointF &off,
                       const QSizeF &size,
                       qreal mnd,
                       bool st,
                       const S57::ElementDataVector &elems,
                       const S52::ColorVector &colors)
  : d(new SymbolDataPrivate)
{
  d->offset = off;
  d->size = size;
  d->elements = elems;
  d->colors = colors;
  d->computeAdvance(mnd, st);
}

SymbolData::~SymbolData() {}


bool SymbolData::isValid() const {
  return d->size.isValid();
}

const QPointF& SymbolData::offset() const {
  return d->offset;
}

const QSizeF& SymbolData::size() const {
  return d->size;
}

const PatternMMAdvance& SymbolData::advance() const {
  return d->advance;
}

const S57::ElementData& SymbolData::element() const {
  return d->elements.first();
}

const S57::ElementDataVector& SymbolData::elements() const {
  return d->elements;
}

const S52::ColorVector& SymbolData::colors() const {
  return d->colors;
}

bool SymbolData::operator== (const SymbolData& s2) const {
  if (d->offset != s2.d->offset) return false;
  if (d->size != s2.d->size) return false;
  if (d->advance.x != s2.d->advance.x) return false;
  if (d->advance.xy != s2.d->advance.xy) return false;
  if (d->elements.size() != s2.d->elements.size()) return false;
  for (int i = 0; i < d->elements.size(); i++) {
    if (d->elements[i].offset != s2.d->elements[i].offset) return false;
  }
  return true;
}
