/* -*- coding: utf-8-unix -*-
 *
 * File: src/hpglparser.cpp
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
#include "hpglpixmapparser.h"
#include <QDebug>
#include "s52presentation.h"
#include <QPainter>
#include "settings.h"
#include "platform.h"




HPGL::PixmapParser::PixmapParser(const QString& src, const QString& colors,
                                 const QPointF& pivot, qint16 angle)
  : Parser(colors)
  , m_pivot(pivot)
  , m_started(false)
  , m_penDown(false)
{

  parse(src);

  if (!m_ok) {
    return;
  }

  while (m_sketches.size() > 0) {
    edgeSketch();
  }
  edgeSketch();

  if (angle != 0) {
    QTransform rot;
    rot.rotate(angle);

    for (PainterItem& item: m_items) {
      for (QPolygonF& p: item.path) {
        p = rot.map(p);
        m_bbox |= p.boundingRect();
      }
    }
  } else {
    for (const PainterItem& item: m_items) {
      for (const QPolygonF& p: item.path) {
        m_bbox |= p.boundingRect();
      }
    }
  }


  int w = std::ceil(m_bbox.width());
  int h = std::ceil(m_bbox.height());
  m_pix = QPixmap(w, h);
  m_pix.fill(QColor("transparent"));

  QTransform tr;
  tr.translate(-m_bbox.left(), -m_bbox.top());
  paint(tr);
}

HPGL::PixmapParser::PixmapParser(const QString& src, const QString& colors,
                                 const QPointF& pivot, bool isLine)
  : Parser(colors)
  , m_pivot(pivot)
  , m_started(false)
  , m_penDown(false)
{

  parse(src);

  if (!m_ok) {
    return;
  }

  while (m_sketches.size() > 0) {
    edgeSketch();
  }
  edgeSketch();

  QRectF bbox;
  for (const PainterItem& item: m_items) {
    for (const QPolygonF& p: item.path) {
      bbox |= p.boundingRect();
    }
  }

  const auto s0 = isLine ? .45 : .9;
  auto s = 1.;
  if (bbox.width() / PickIconSize > s0) s = s0;
  const auto hmax = 7.;
  if (!isLine && bbox.height() > hmax) {
    s = std::min(s, hmax / bbox.height());
  }


  m_bbox = QRectF(0, 0, PickIconSize, PickIconSize);
  m_pix = QPixmap(PickIconSize, PickIconSize);
  m_pix.fill(QColor("transparent"));

  if (isLine) {
    const auto nmax = static_cast<int>(std::ceil(PickIconSize * 1.4 / s / bbox.width()));

    QTransform op;
    op.rotate(45);
    op.scale(s, s);

    const auto p0 = - op.map(bbox.topLeft() + QPointF(0, .5 * bbox.height()));
    op = op * QTransform(1, 0, 0, 1, p0.x(), p0.y());

    const auto T = (s * bbox.width() + 1) / sqrt(2) * QPointF(1., 1.);
    for (int n = 0; n < nmax; n++) {
      paint(op);
      op = op * QTransform(1, 0, 0, 1, T.x(), T.y());
    }
  } else {

    const auto nmax = static_cast<int>(std::ceil(PickIconSize / s / bbox.width()));

    QTransform op;
    op.scale(s, s);

    auto p0 = - op.map(bbox.topLeft());
    op = op * QTransform(1, 0, 0, 1, p0.x(), p0.y());

    auto T = (s * bbox.width() + 1) * QPointF(1., 0.);
    for (int n = 0; n < nmax; n++) {
      paint(op);
      op = op * QTransform(1, 0, 0, 1, T.x(), T.y());
    }

    op.reset();
    op.rotate(180);
    op.scale(s, s);

    p0 = QPointF(PickIconSize, PickIconSize) - op.map(bbox.topLeft());
    op = op * QTransform(1, 0, 0, 1, p0.x(), p0.y());

    T = (s * bbox.width() + 1) * QPointF(-1., 0.);
    for (int n = 0; n < nmax; n++) {
      paint(op);
      op = op * QTransform(1, 0, 0, 1, T.x(), T.y());
    }

    if (!m_items.isEmpty()) {
      QPainter painter(&m_pix);
      painter.setPen(m_items.first().pen);
      const auto dy = s * bbox.height() / 2;
      const auto dx = painter.pen().widthF();
      painter.drawLine(dx, dy, dx, PickIconSize - dy);
      painter.drawLine(PickIconSize - dx, dy, PickIconSize - dx, PickIconSize - dy);
    }
  }
}

void HPGL::PixmapParser::paint(const QTransform& tr) {
  QPainter painter(&m_pix);
  for (const PainterItem& item: m_items) {
    painter.setPen(item.pen);
    painter.setBrush(item.brush);
    for (const QPolygonF& p: item.path) {
      if (p.isClosed()) {
        painter.drawPolygon(tr.map(p));
      } else {
        painter.drawPolyline(tr.map(p));
      }
    }
  }
}



void HPGL::PixmapParser::setColor(char c) {
  if (!m_cmap.contains(c)) {
    qWarning() << c << "not in colormap";
    m_ok = false;
    return;
  }
  const quint32 index = m_cmap[c];
  if (!m_started) {
    m_started = true;
    m_currentSketch.color = S52::Color(index);
    m_currentSketch.lineWidth = 0.;
    LineString ls;
    ls.closed = false;
    m_currentSketch.parts.append(ls);
  } else if (m_currentSketch.color.index != index) {
    Sketch sketch = m_currentSketch.clone();
    sketch.color.index = index;
    m_sketches.push(m_currentSketch);
    m_currentSketch = sketch;
  }
}

void HPGL::PixmapParser::setAlpha(int a) {
  auto alpha = as_enum<S52::Alpha>(a, S52::AllAlphas);
  if (m_currentSketch.color.alpha == S52::Alpha::Unset) {
    m_currentSketch.color.alpha = alpha;
  } else if (m_currentSketch.color.alpha != alpha) {
    Sketch sketch = m_currentSketch.clone();
    sketch.color.alpha = alpha;
    m_sketches.push(m_currentSketch);
    m_currentSketch = sketch;
  }
}

static qreal makeLW(int w) {
  return S52::LineWidthMM(w) * dots_per_mm_x() * Settings::instance()->displayLineWidthScaling();
}


void HPGL::PixmapParser::setWidth(int w) {
  const qreal lw = makeLW(w);
  if (m_currentSketch.lineWidth == 0.) {
    m_currentSketch.lineWidth = lw;
  } else if (m_currentSketch.lineWidth != lw) {
    Sketch sketch = m_currentSketch.clone();
    sketch.lineWidth = lw;
    m_sketches.push(m_currentSketch);
    m_currentSketch = sketch;
  }
}

void HPGL::PixmapParser::movePen(const RawPoints &ps) {
  if (ps.size() % 2 != 0) {
    qWarning() << ps << "contains odd number of coordinates";
    m_ok = false;
    return;
  }

  m_penPos = makePoint(ps[ps.size() - 2], ps[ps.size() - 1]);

  LineString ls;
  ls.closed = false;
  m_currentSketch.parts.append(ls);
  m_penDown = false;
}

void HPGL::PixmapParser::drawLineString(const RawPoints &ps) {
  if (ps.isEmpty()) {
    // draw a single point
    drawPoint();
    m_penDown = true;
    return;
  }

  if (ps.size() % 2 != 0) {
    qWarning() << ps << "contains odd number of coordinates";
    m_ok = false;
    return;
  }

  if (!m_penDown) {
    m_currentSketch.parts.last().points.append(m_penPos);
  }
  for (int i = 0; i < ps.size() / 2; i++) {
    m_currentSketch.parts.last().points.append(makePoint(ps[2 * i], ps[2 * i + 1]));
  }

  m_currentSketch.parts.last().closed =
      m_currentSketch.parts.last().points.first() ==
      m_currentSketch.parts.last().points.last();

  m_penPos = m_currentSketch.parts.last().points.last();
  m_penDown = true;
}

void HPGL::PixmapParser::drawPoint() {
  const qreal lw = m_currentSketch.lineWidth == 0. ? makeLW(1) : m_currentSketch.lineWidth;
  const auto q = QPointF(lw / 2, lw / 2);
  const auto p = QPointF(lw / 2, -lw / 2);
  QPolygonF poly;
  poly.append(m_penPos - q);
  poly.append(m_penPos + p);
  poly.append(m_penPos + q);
  poly.append(m_penPos - p);
  poly.append(m_penPos - q);

  m_items.append(PainterItem(poly, QBrush(S52::GetColor(m_currentSketch.color.index))));
}


void HPGL::PixmapParser::drawCircle(int r0) {
  LineString ls;
  ls.closed = true;
  const qreal lw = m_currentSketch.lineWidth == 0 ? makeLW(1) : m_currentSketch.lineWidth;

  const qreal r = mmUnit * r0 * dots_per_mm_x() / Settings::instance()->displayLengthScaling();

  auto n = qMin(90, 3 + 4 * static_cast<int>(r / lw));
  for (int i = 0; i <= n; i++) {
    const qreal a = 2 * i * M_PI / n;
    const QPointF p = m_penPos + QPointF(r * cos(a), r * sin(a));
    ls.points.append(p);
  }
  m_currentSketch.parts.append(ls);
  LineString ls2;
  ls2.closed = false;
  if (m_penDown) {
    ls2.points.append(m_penPos);
  }
  m_currentSketch.parts.append(ls2);
}

void HPGL::PixmapParser::drawArc(int x0, int y0, int a0) {
  LineString ls;
  ls.closed = false;
  const qreal lw = m_currentSketch.lineWidth == 0 ? makeLW(1) : m_currentSketch.lineWidth;

  const QPointF c = makePoint(x0, y0);
  const QPointF d = m_penPos - c;
  const qreal r = sqrt(QPointF::dotProduct(d, d));
  const int n = qMin(90., std::ceil((3. + 4 * r / lw) * a0 / 360.));
  for (int i = 0; i <= n; i++) {
    const qreal a = a0 / 180. * i  * M_PI / n;
    const qreal ca = cos(a);
    const qreal sa = sin(a);
    const QPointF p = c + QPointF(ca * d.x() - sa * d.y(), sa * d.x() + ca * d.y());
    ls.points.append(p);
  }
  m_currentSketch.parts.append(ls);
  LineString ls2;
  ls2.closed = false;
  if (m_penDown) {
    ls2.points.append(m_penPos);
  }
  m_currentSketch.parts.append(ls2);
}

void HPGL::PixmapParser::pushSketch() {
  Sketch sketch = m_currentSketch.clone();
  m_sketches.push(m_currentSketch);
  m_currentSketch = sketch;
}

void HPGL::PixmapParser::endSketch() {
  // noop
}

void HPGL::PixmapParser::fillSketch() {
  auto c = S52::GetColor(m_currentSketch.color.index);
  if (m_currentSketch.color.alpha != S52::Alpha::Unset) {
    c.setAlpha(255 - as_numeric(m_currentSketch.color.alpha) * 255 / 4);
  }

  PainterItem item;
  item.brush = QBrush(c);

  for (const LineString& part: m_currentSketch.parts) {
    if (!part.closed) continue;
    if (part.points.size() < 3) continue;
    item.path.append(part.points);
  }

  m_items.append(item);

  // pop sketch
  if (!m_sketches.isEmpty()) {
    m_currentSketch = m_sketches.pop();
  }
}


void HPGL::PixmapParser::edgeSketch() {
  auto c = S52::GetColor(m_currentSketch.color.index);
  if (m_currentSketch.color.alpha != S52::Alpha::Unset) {
    c.setAlpha(255 - as_numeric(m_currentSketch.color.alpha) * 255 / 4);
  }
  const qreal lw = m_currentSketch.lineWidth == 0. ? makeLW(1) : m_currentSketch.lineWidth;

  PainterItem item;
  item.pen = QPen(c, std::max(1, static_cast<int>(lw)));

  for (const LineString& part: m_currentSketch.parts) {
    if (part.points.size() < 2) continue;
    item.path.append(part.points);
  }

  m_items.append(item);

  // pop sketch
  if (!m_sketches.isEmpty()) {
    m_currentSketch = m_sketches.pop();
  }
}

QPointF HPGL::PixmapParser::makePoint(int x, int y) {
  const QPointF p(x * mmUnit * dots_per_mm_x(), y * mmUnit * dots_per_mm_y());
  return (p - m_pivot) / Settings::instance()->displayLengthScaling();
}


