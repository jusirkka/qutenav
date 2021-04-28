/* -*- coding: utf-8-unix -*-
 *
 * File: src/tiny_sdf.cpp
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
#include "tiny_sdf.h"
#include <cmath>

TinySDF::TinySDF(const FT_Bitmap& bmap, uint pad)
  : TinySDF(bmap.width,
            bmap.rows,
            std::max(bmap.width, bmap.rows),
            bmap.buffer,
            pad)
{}

TinySDF::~TinySDF() {
  delete [] m_sdf;
}

static const double INF = 1e20;

TinySDF::TinySDF(uint w, uint h, uint maxdim, const uchar *bytes, uint pad)
  : m_size(w + 2 * pad, h + 2 * pad)
  , m_func(maxdim + 2 * pad)
  , m_parabola(maxdim + 2 * pad)
  , m_envelope(maxdim + 2 * pad + 1)
  , m_dist2(maxdim + 2 * pad)
  , m_gridOuter(m_size.height() * m_size.width(), INF)
  , m_gridInner(m_size.height() * m_size.width(), 0.)
  , m_sdf(new uchar[m_size.height() * m_size.width()])
{
  const uint offset = pad * m_size.width() + pad;
  const uint stride = m_size.width();

  for (uint row = 0; row < h; row++) {
    for (uint col = 0; col < w; col++) {
      const double a = double(bytes[row * w + col]) / 255;
      m_gridOuter[offset + row * stride + col] = a == 1.0 ? 0.0 : a == 0.0 ? INF : std::pow(std::max(0.0, 0.5 - a), 2.0);
      m_gridInner[offset + row * stride + col] = a == 1.0 ? INF : a == 0.0 ? 0.0 : std::pow(std::max(0.0, a - 0.5), 2.0);
    }
  }

  edt(m_gridOuter);
  edt(m_gridInner);
}

const uchar* TinySDF::transform(double radius, double cutoff) {
  const uint size = m_size.width() * m_size.height();
  for (uint i = 0; i < size; i++) {
    const double distance = m_gridOuter[i] - m_gridInner[i];
    m_sdf[i] = std::max(0l, std::min(255l, ::lround(255.0 - 255.0 * (distance / radius + cutoff))));
  }

  return m_sdf;
}


// 2D Euclidean distance transform by Felzenszwalb & Huttenlocher https://cs.brown.edu/~pff/dt/
void TinySDF::edt(QVector<double>& grid) {
  const uint w = m_size.width();
  const uint h = m_size.height();

  for (uint x = 0; x < w; x++) {
    for (uint y = 0; y < h; y++) {
      m_func[y] = grid[y * w + x];
    }
    edt1d(h);
    for (uint y = 0; y < h; y++) {
      grid[y * w + x] = m_dist2[y];
    }
  }
  for (uint y = 0; y < h; y++) {
    for (uint x = 0; x < w; x++) {
      m_func[x] = grid[y * w + x];
    }
    edt1d(w);
    for (uint x = 0; x < w; x++) {
      grid[y * w + x] = std::sqrt(m_dist2[x]);
    }
  }
}

// 1D squared distance transform
void TinySDF::edt1d(uint n) {
  m_parabola[0] = 0;
  m_envelope[0] = -INF;
  m_envelope[1] = +INF;

  for (uint q = 1, k = 0; q < n; q++) {
    double isect = intersection(q, k);
    while (isect <= m_envelope[k]) {
      k--;
      isect = intersection(q, k);
    }
    k++;
    m_parabola[k] = q;
    m_envelope[k] = isect;
    m_envelope[k + 1] = +INF;
  }

  for (uint q = 0, k = 0; q < n; q++) {
    while (m_envelope[k + 1] < q) k++;
    const double v = m_parabola[k];
    const double d = v - q;
    m_dist2[q] = d * d + m_func[v];
  }
}


double TinySDF::intersection(uint q, uint k) const {
  const double v = m_parabola[k];
  return ((q * q + m_func[q]) - (v * v + m_func[v])) / (2 * q - 2 * v);
}

