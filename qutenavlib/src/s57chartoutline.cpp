/* -*- coding: utf-8-unix -*-
 *
 * File: src/s57chartoutline.cpp
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
#include "s57chartoutline.h"
#include "s57chartoutline_p.h"

S57ChartOutline::S57ChartOutline()
  : d(new S57ChartOutlinePrivate)
{}

S57ChartOutline::S57ChartOutline(S57ChartOutlinePrivate *d)
  : d(d)
{}

S57ChartOutline::S57ChartOutline(const WGS84Point& sw,
                                 const WGS84Point& ne,
                                 const Region& cov,
                                 const Region& nocov,
                                 quint32 scale,
                                 const QDate& pub,
                                 const QDate& mod)
  : d(new S57ChartOutlinePrivate)
{
  d->extent = Extent(sw, ne);
  d->cov = cov;
  d->nocov = nocov;
  d->center = WGS84Point::fromLL(.5 * (sw.lng() + ne.lng()),
                                 .5 * (sw.lat() + ne.lat()));

  d->scaling = QSizeF(1., 1.);
  d->scale = scale;
  d->pub = pub;
  d->mod = mod;
}

S57ChartOutline::S57ChartOutline(const WGS84Point& sw,
                                 const WGS84Point& ne,
                                 const Region& cov,
                                 const Region& nocov,
                                 const WGS84Point& ref,
                                 const QSizeF& scaling,
                                 quint32 scale,
                                 const QDate& pub,
                                 const QDate& mod)
  : d(new S57ChartOutlinePrivate)
{
  d->extent = Extent(sw, ne);
  d->cov = cov;
  d->nocov = nocov;
  d->center = ref;
  d->scaling = scaling;
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

const Region& S57ChartOutline::coverage() const {
  return d->cov;
}

const Region& S57ChartOutline::nocoverage() const {
  return d->nocov;
}

const WGS84Point& S57ChartOutline::reference() const {
  return d->center;
}

const QSizeF& S57ChartOutline::scaling() const {
  return d->scaling;
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
