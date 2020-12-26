#include "s57chartoutline.h"
#include "s57chartoutline_p.h"

S57ChartOutline::S57ChartOutline()
  : d(new S57ChartOutlinePrivate)
{}

S57ChartOutline::S57ChartOutline(S57ChartOutlinePrivate *d)
  : d(d)
{}

S57ChartOutline::S57ChartOutline(const WGS84Point& sw,
                                 const WGS84Point& se,
                                 const WGS84Point& ne,
                                 const WGS84Point& nw,
                                 quint32 scale,
                                 const QDate& pub,
                                 const QDate& mod)
  : d(new S57ChartOutlinePrivate)
{
  d->extent = Extent(sw, se, ne, nw);
  d->center = WGS84Point::fromLL(.5 * (nw.lng() + se.lng()),
                                 .5 * (nw.lat() + se.lat()));

  d->scale = scale;
  d->pub = pub;
  d->mod = mod;
}

S57ChartOutline::S57ChartOutline(const S57ChartOutline& s)
  : d(s.d)
{}

S57ChartOutline& S57ChartOutline::operator=(const S57ChartOutline& s) {
  d = s.d;
  return *this;
}


S57ChartOutline::~S57ChartOutline() {}

const Extent& S57ChartOutline::extent() const {
  return d->extent;
}

const WGS84Point& S57ChartOutline::reference() const {
  return d->center;
}

quint32 S57ChartOutline::scale() const {
  return d->scale;
}

const QDate& S57ChartOutline::published() const {
  return d->pub;
}

const QDate& S57ChartOutline::modified() const {
  return d->mod;
}
