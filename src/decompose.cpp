/* -*- coding: utf-8-unix -*-
 *
 * File: src/decompose.cpp
 *
 * Copyright (C) 2022 Jukka Sirkka
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

#include "decompose.h"
#include <QMap>
#include "logging.h"
#include <cmath>
#include <QFile>

KV::Decomposer::Decomposer(const PointVector& ps, qreal prec, bool outer) {
  // qCDebug(CREG) << qSetRealNumberPrecision(8) << "initial polygon" << ps;
  IndexVector indices = findDiagonals(ps);
  QMap<int, PointVector> dss;
  for (int index: indices) {
    dss[index] = approximateDiagonal(ps, index, prec, outer);
    // qCDebug(CREG) << "new points" << qSetRealNumberPrecision(8) << dss[index];
  }

  PointVector qs;
  if (!indices.isEmpty()) {
    int i0 = 0;
    for (int i1: indices) {
      // qCDebug(CREG) << "add points" << i0 << i1;
      addPoints(qs, ps, dss[i1], i0, i1);
      i0 = i1 + 1;
    }
    // add last points
    // qCDebug(CREG) << "last points" << i0 << ps.size();
    for (int i = i0; i < ps.size(); ++i) {
      qs << ps[i];
    }
  } else {
    qs = ps;
  }
  // qCDebug(CREG) << qSetRealNumberPrecision(8) << "rectangular polygon" << qs;
  while (reduce(qs)); // intentionally no body
  // qCDebug(CREG) << qSetRealNumberPrecision(8) << "reduced polygon" << qs;

  QMap<qreal, XVector> bin;
  for (const QPointF& q: qs) {
    bin[q.y()].append(q.x());
  }

  for (auto it = bin.begin(); it != bin.end(); ++it) {
    XVector xs = it.value();
    if (xs.size() % 2 == 1) {
      qCDebug(CREG) << "-- conflict --" << xs << it.key();
    }
    Q_ASSERT(xs.size() % 2 == 0);
    std::sort(xs.begin(), xs.end());
    Strip s;
    s.y = it.key();
    s.xs = xs;
    m_strips.append(s);
  }
  std::sort(m_strips.begin(), m_strips.end(), [] (const Strip& s1, const Strip& s2) {
    return s1.y < s2.y;
  });
}

QRectF KV::Decomposer::nextRect() {
  const auto y0 = m_strips.first().y;
  const auto x0 = m_strips.first().xs[0];
  const auto x1 = m_strips.first().xs[1];

  auto it = m_strips.cbegin();
  ++it;
  bool done = false;
  qreal y1;
  while (!done) {
    const auto& xs = it->xs;
    for (const auto x: xs) {
      if (x < x0) continue;
      if (x > x1) break;
      done = true;
      y1 = it->y;
      break;
    }
    ++it;
  }
  m_strips.first().xs.removeFirst();
  m_strips.first().xs.removeFirst();
  if (m_strips.first().xs.isEmpty()) {
    m_strips.removeFirst();
  }
  auto bookKeeper = [this, y1] (qreal x) {
    int ix = -1;
    int iy = -1;
    find(QPointF(x, y1), ix, iy);
    Q_ASSERT(iy > -1);
    if (ix > -1) {
      erase(ix, iy);
    } else {
      insert(iy, x);
    }
  };
  bookKeeper(x0);
  bookKeeper(x1);

  return QRectF(QPointF(x0, y0), QPointF(x1, y1));
}

void KV::Decomposer::find(const QPointF& p, int& ix, int& iy) const {
  const qreal x = p.x();
  const qreal y = p.y();
  for (int i = 0; i < m_strips.size(); ++i) {
    if (m_strips[i].y < y) continue;
    if (m_strips[i].y != y) return;
    iy = i;
    const auto xs = m_strips[i].xs;
    for (int j = 0; j < xs.size(); ++j) {
      if (xs[j] < x) continue;
      if (xs[j] != x) return;
      ix = j;
      return;
    }
    return;
  }
}

void KV::Decomposer::erase(int ix, int iy) {
  m_strips[iy].xs.removeAt(ix);
  if (m_strips[iy].xs.isEmpty()) {
    m_strips.removeAt(iy);
  }
}

void KV::Decomposer::insert(int iy, qreal x) {
  auto& xs = m_strips[iy].xs;
  for (int j = 0; j < xs.size(); ++j) {
    if (xs[j] < x) continue;
    xs.insert(j, x);
    return;
  }
  xs.append(x);
}

KV::IndexVector KV::Decomposer::findDiagonals(const PointVector& ps) {
  IndexVector out;
  const int n = ps.size();
  for (int k = 0; k < n; k++) {
    const QPointF p0 = ps[k];
    const QPointF p1 = ps[(k + 1) % n];
    if (p0.x() != p1.x() && p0.y() != p1.y()) {
      out << k;
    }
  }
  return out;
}

void KV::Decomposer::addPoints(PointVector& qs, const PointVector& ps, const PointVector& ds,
                               int i0, int i1) {
  for (int i = i0; i <= i1; ++i) {
    qs << ps[i];
  }
  qs.append(ds);
}

static QPointF get_next(const QPointF& p0, KV::PointVector& ps, bool* found) {

  *found = true;

  while (!ps.isEmpty()) {
    const auto p1 = ps.takeFirst();
    if (p0.x() != p1.x() || p0.y() != p1.y()) {
      return p1;
    }
  }

  *found = false;
  return QPointF();
}

static bool accept(const QPointF& x0, const QPointF& x1, const QPointF& x2, bool& pass2) {
  const auto d0 = x1 - x0;
  const auto d1 = x2 - x1;

  const bool v_lined = d0.x() == 0 && d1.x() == 0;
  if (v_lined) {
    pass2 = pass2 || (d0.y() * d1.y() <= 0);
    return false;
  }

  const bool h_lined = d0.y() == 0 && d1.y() == 0;
  if (h_lined) {
    pass2 = pass2 || (d0.x() * d1.x() <= 0);
    return false;
  }

  return true;
}

bool KV::Decomposer::reduce(PointVector& ps) {
  PointVector out;

  // const auto np = ps.size();

  auto x0 = ps.takeFirst();
  ps << x0;
  out << x0;

  bool found;
  bool pass2 = false;
  auto x1 = get_next(x0, ps, &found);
  if (!found) return pass2;

  while (!ps.isEmpty()) {
    auto x2 = get_next(x1, ps, &found);
    if (!found) continue;
    if (accept(x0, x1, x2, pass2)) {
      x0 = x1;
      out << x0;
    }
    x1 = x2;
  }
  if (!accept(out.last(), out[0], out[1], pass2)) {
    out.removeFirst();
  }
  ps = out;
  // qCDebug(CREG) << "point reduction" << np << "->" << ps.size();
  return pass2;
}

// https://wrf.ecse.rpi.edu//Research/Short_Notes/pnpoly.html
static bool inpolygon(const KV::PointVector& ps, const QPointF& p) {
  const qreal x = p.x();
  const qreal y = p.y();
  bool c = false;
  const int n = ps.size();
  for (int i = 0, j = n - 1; i < n; j = i++) {
    if (((ps[i].y() > y) != (ps[j].y() > y)) &&
        (x < (ps[j].x() - ps[i].x()) * (y - ps[i].y()) / (ps[j].y() - ps[i].y()) + ps[i].x())) {
      c = !c;
    }
  }
  return c;
}

KV::PointVector KV::Decomposer::approximateDiagonal(const PointVector& ps, int index, qreal prec, bool outer) {
  PointVector out;
  const QPointF first = ps[index];
  const QPointF last = ps[(index + 1) % ps.size()];
  const QPointF dp = last - first;
  const qreal d = std::min(std::abs(dp.x()), std::abs(dp.y()));
  const int n = std::min(6, static_cast<int>(std::ceil(d / prec)));
  // const int n = static_cast<int>(std::ceil(d / prec));
  // qCDebug(CREG) << "number of new points =" << 2 * n - 1;
  const bool h_in = inpolygon(ps, first + QPointF(dp.x() / n * 0.01, 0));
  const bool h_first = (h_in && !outer) || (!h_in && outer);
  // qCDebug(CREG) << "h_first =" << h_first << "segment =" << qSetRealNumberPrecision(8) << first << last;

  auto curr = first;
  if (h_first) {
    curr += QPointF(dp.x() / n, 0);
    out << curr;
  }

  for (int i = 0; i < n - 1; ++i) {
    curr += QPointF(0, dp.y() / n);
    out << curr;
    curr += QPointF(dp.x() / n, 0);
    out << curr;
  }

  if (!h_first) {
    curr += QPointF(0, dp.y() / n);
    out << curr;
  }

  // ensure that first and last points are consistent: account rounding errors
  if (h_first) {
    out.first().ry() = first.y();
    out.last().rx() = last.x();
  } else {
    out.first().rx() = first.x();
    out.last().ry() = last.y();
  }

  return out;
}
