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

HPGL::PixmapParser::PixmapParser(const QString &src, const QString& colors, qint16 angle)
  : Parser(colors)
  , m_pix()
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

  if (angle != 0) {
    const auto c = bbox.center();
    QTransform rot;
    rot.translate(c.x(), c.y());
    rot.rotate(angle);
    rot.translate(-c.x(), -c.y());

    bbox = QRectF();
    for (PainterItem& item: m_items) {
      for (QPolygonF& p: item.path) {
        p = rot.map(p);
        bbox |= p.boundingRect();
      }
    }
  }


  int w = bbox.width() + .5;
  int h = bbox.height() + .5;
  m_pix = QPixmap(w, h);
  m_pix.fill(QColor("transparent"));

  QPainter painter(&m_pix);
  for (const PainterItem& item: m_items) {
    painter.setPen(item.pen);
    painter.setBrush(item.brush);
    for (const QPolygonF& p: item.path) {
      painter.drawPolygon(p.translated(- bbox.topLeft()));
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

void HPGL::PixmapParser::setWidth(int w) {
  const qreal lw = S52::LineWidthMM(w) * dots_per_mm_x();
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
  const qreal lw = m_currentSketch.lineWidth == 0. ?
        S52::LineWidthMM(1) * dots_per_mm_x() : m_currentSketch.lineWidth;
  const auto q = QPointF(lw / 2, lw / 2);
  const auto p = QPointF(lw / 2, -lw / 2);
  QPolygonF poly;
  poly.append(m_penPos - q);
  poly.append(m_penPos + p);
  poly.append(m_penPos + q);
  poly.append(m_penPos - p);

  m_items.append(PainterItem(poly, QBrush(S52::GetColor(m_currentSketch.color.index))));
}


void HPGL::PixmapParser::drawCircle(int r0) {
  LineString ls;
  ls.closed = true;
  const qreal lw = m_currentSketch.lineWidth == 0 ?
        S52::LineWidthMM(1) * dots_per_mm_x() : m_currentSketch.lineWidth;

  const qreal r = mmUnit * r0 * dots_per_mm_x();

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
  const qreal lw = m_currentSketch.lineWidth == 0 ?
        S52::LineWidthMM(1) * dots_per_mm_x() : m_currentSketch.lineWidth;

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
  const qreal lw = m_currentSketch.lineWidth == 0. ?
        S52::LineWidthMM(1) * dots_per_mm_x() : m_currentSketch.lineWidth;

  PainterItem item;
  item.pen = QPen(c, lw + .5);

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
  QPointF p(x * mmUnit * dots_per_mm_x(), y * mmUnit * dots_per_mm_y());
  return p;
}


