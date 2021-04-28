/* -*- coding: utf-8-unix -*-
 *
 * File: src/symboldata.h
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

#include "types.h"
#include "s57object.h"
#include <QSharedDataPointer>

class SymbolDataPrivate;


class SymbolData {
public:

  SymbolData(SymbolDataPrivate *d);
  SymbolData();
  SymbolData(const QPointF& off, const QSizeF& size, qreal mnd, bool st,
             const S57::ElementData& elem);
  SymbolData(const QPointF& off, const QSizeF& size, qreal mnd, bool st,
             const S57::ElementDataVector& elems, const S52::ColorVector& colors);
  SymbolData(const SymbolData& s);

  SymbolData& operator=(const SymbolData& s);
  SymbolData& operator=(SymbolData&& s) noexcept {qSwap(d, s.d); return *this;}

  ~SymbolData();

  bool operator==(const SymbolData& s) const;
  inline bool operator!=(const SymbolData& s) const {return !(operator==(s));}

  bool isValid() const;
  const QPointF& offset() const;
  const QSizeF& size() const;
  const PatternMMAdvance& advance() const;
  const S57::ElementData& element() const;
  const S57::ElementDataVector& elements() const;
  const S52::ColorVector& colors() const;

private:

  QSharedDataPointer<SymbolDataPrivate> d;

};


