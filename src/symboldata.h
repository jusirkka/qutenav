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


