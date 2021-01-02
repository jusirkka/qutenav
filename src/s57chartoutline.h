#pragma once

#include "types.h"
#include <QSharedDataPointer>



class S57ChartOutlinePrivate;

class S57ChartOutline {
public:

  S57ChartOutline(S57ChartOutlinePrivate *d);
  S57ChartOutline();
  S57ChartOutline(const WGS84Point& sw,
                  const WGS84Point& se,
                  const WGS84Point& ne,
                  const WGS84Point& nw,
                  quint32 scale,
                  const QDate& pub,
                  const QDate& mod);

  S57ChartOutline(const WGS84Point& sw,
                  const WGS84Point& ne,
                  const WGS84Point& ref,
                  const QSizeF& scaling,
                  quint32 scale,
                  const QDate& pub,
                  const QDate& mod);

  S57ChartOutline(const S57ChartOutline& s);

  S57ChartOutline& operator=(const S57ChartOutline& s);
  S57ChartOutline& operator=(S57ChartOutline&& s) noexcept {qSwap(d, s.d); return *this;}

  ~S57ChartOutline();

  const Extent& extent() const;
  const WGS84Point& reference() const;
  const QSizeF& scaling() const;
  quint32 scale() const;
  const QDate& published() const;
  const QDate& modified() const;

private:

  QSharedDataPointer<S57ChartOutlinePrivate> d;

};

