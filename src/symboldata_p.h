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

  inline void computeAdvance(int mnd, bool st) {
    // check if pivot is outside of the rect(offset, w, h) and enlarge
    // note: inverted y-axis
    const QPointF origin(offset.x(), offset.y() - size.height());
    const QRectF rtest(origin, size);
    auto r1 = rtest.united(QRectF(QPointF(0, 0), QSizeF(.1, .1)));
    const int x0 = r1.width() + mnd;
    const int y0 = r1.height() + mnd;
    const int x1 = st ? .5 * x0 : 0.;
    advance = PatternAdvance(x0, y0, x1);
  }

  QPoint offset;
  QSize size;
  PatternAdvance advance;
  S57::ElementDataVector elements;
  ColorVector colors;
};
