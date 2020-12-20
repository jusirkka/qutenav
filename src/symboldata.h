#pragma once

#include "types.h"
#include "s57object.h"
#include <QSharedDataPointer>

class SymbolDataPrivate;


class SymbolData {
public:

  using ColorVector = QVector<S52::Color>;

  SymbolData(SymbolDataPrivate *d);
  SymbolData();
  SymbolData(const QPoint& off, const QSize& size, int mnd, bool st,
             const S57::ElementData& elem);
  SymbolData(const QPoint& off, const QSize& size, int mnd, bool st,
             const S57::ElementDataVector& elems, const ColorVector& colors);
  SymbolData(const SymbolData& s);

  SymbolData& operator=(const SymbolData& s);
  SymbolData& operator=(SymbolData&& s) noexcept {qSwap(d, s.d); return *this;}

  ~SymbolData();

  bool operator==(const SymbolData& s) const;
  inline bool operator!=(const SymbolData& s) const {return !(operator==(s));}

  bool isValid() const;
  const QPoint& offset() const;
  const QSize& size() const;
  const PatternAdvance& advance() const;
  const S57::ElementData& element() const;
  const S57::ElementDataVector& elements() const;
  const ColorVector& colors() const;

private:

  QSharedDataPointer<SymbolDataPrivate> d;

};


