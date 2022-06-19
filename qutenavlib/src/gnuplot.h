/* -*- coding: utf-8-unix -*-
 *
 * gnuplot.h
 *
 * Created: 2021-03-31 2021 by Jukka Sirkka
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
#pragma once

#include "region.h"
#include "geoprojection.h"
#include <QFile>
#include <QTextStream>

namespace chartcover {

using LLPolygon = QVector<WGS84PointVector>;

inline void tognuplot(const LLPolygon& cov, const LLPolygon& nocov,
                      const WGS84Point& sw, const WGS84Point& ne,
                      const GeoProjection* gp,
                      const QString& fname,
                      bool onefile = false,
                      const KV::Region& reg = KV::Region()) {

  const QString gpath = QString("%1.gnuplot").arg(fname);
  QFile file(gpath);
  if (onefile) {
    file.open(QFile::ReadWrite);
  } else {
    file.open(QFile::WriteOnly);
  }
  QTextStream stream(&file);
  if (onefile) {
    stream.seek(file.size());
  }


  QPointF p;
  stream << "\n";
  p = gp->fromWGS84(sw);
  stream << p.x() << " " << p.y() << " 0\n";

  p = gp->fromWGS84(WGS84Point::fromLL(ne.lng(), sw.lat()));
  stream << p.x() << " " << p.y() << " 0\n";

  p = gp->fromWGS84(ne);
  stream << p.x() << " " << p.y() << " 0\n";

  p = gp->fromWGS84(WGS84Point::fromLL(sw.lng(), ne.lat()));
  stream << p.x() << " " << p.y() << " 0\n";

  p = gp->fromWGS84(sw);
  stream << p.x() << " " << p.y() << " 0\n\n";

  for (const WGS84PointVector& ws: cov) {
    for (const WGS84Point& w: ws) {
      p = gp->fromWGS84(w);
      stream << p.x() << " " << p.y() << " 1\n";
    }
    p = gp->fromWGS84(ws.first());
    stream << p.x() << " " << p.y() << " 1\n\n";
  }

  for (const WGS84PointVector& ws: nocov) {
    for (const WGS84Point& w: ws) {
      p = gp->fromWGS84(w);
      stream << p.x() << " " << p.y() << " 2\n";
    }
    p = gp->fromWGS84(ws.first());
    stream << p.x() << " " << p.y() << " 2\n\n";
  }

  qDebug() << "Number of boxes" << gpath << reg.rectCount();
  auto rects = reg.rects();
  QRectF prev;
  for (const QRectF& r: rects) {

    qDebug() << r.left() << r.top() << r.width() << r.height() << r.top() - prev.bottom()
             << r.left() - prev.left() << r.right() - prev.right();
    prev = r;

    stream << r.left() << " " << r.top() << " 0\n";
    stream << r.right() << " " << r.top() << " 0\n";
    stream << r.right() << " " << r.bottom() << " 0\n";
    stream << r.left() << " " << r.bottom() << " 0\n";
    stream << r.left() << " " << r.top() << " 0\n\n";
  }

  file.close();
}

} // namespace chartcover


namespace chartmanager {

inline void tognuplot(const KV::RegionMap& regions, const QRectF& va, const QString& basename) {

  using Iter = KV::RegionMap::const_iterator;

  QString base(basename);
  for (Iter it = regions.begin(); it != regions.end(); ++it) {
    base = QString("%1_%2").arg(base).arg(it.key());
  }

  QFile file(base + ".gnuplot");
  // file.open(QFile::ReadWrite);
  file.open(QFile::WriteOnly);
  QTextStream stream(&file);
  // stream.seek(file.size());


  auto plotBox = [&stream] (const QRectF& r, int pal, float s) {
    QVector<QPointF> ps {r.topLeft(), r.topRight(), r.bottomRight(), r.bottomLeft()};
    for (auto p: ps) {
      const QPointF q = s * (p - r.center()) + r.center();
      stream << q.x() << " " << q.y() << " " << pal << "\n";
    }
    const QPointF q0 = s * (ps.first() - r.center()) + r.center();
    stream << q0.x() << " " << q0.y() << " " << pal << "\n\n";
  };

  int k = 0;
  float s = 1.;
  plotBox(va, k, s);

  for (Iter it = regions.begin(); it != regions.end(); ++it) {
    k += 1;
    const KV::Region reg = it.value();
    // qDebug() << "Number of boxes" << it.key() << reg.rectCount();
    auto rects = reg.rects();
    for (const QRectF& r: rects) {
      plotBox(r, k, s);
    }
  }

  file.close();

}

} // namespace chartmanager


namespace paintdata {

inline void tognuplot(const QVector<QRectF>& boxes, const KV::Region& cover) {

  QString path = QString("elems_%1.gnuplot").arg(boxes.size(), 5, 10, QChar('0'));
  QFile file(path);
  // file.open(QFile::ReadWrite);
  file.open(QFile::WriteOnly);
  QTextStream stream(&file);
  // stream.seek(file.size());


  auto plotBox = [&stream] (const QRectF& r, int pal, float s) {
    QVector<QPointF> ps {r.topLeft(), r.topRight(), r.bottomRight(), r.bottomLeft()};
    for (auto p: ps) {
      const QPointF q = s * (p - r.center()) + r.center();
      stream << q.x() << " " << q.y() << " " << pal << "\n";
    }
    const QPointF q0 = s * (ps.first() - r.center()) + r.center();
    stream << q0.x() << " " << q0.y() << " " << pal << "\n\n";
  };

  auto rects = cover.rects();
  for (const QRectF& r: rects) {
    plotBox(r, 0, 1.);
  }

  for (const QRectF& r: boxes) {
    plotBox(r, 1, 1.);
  }

  file.close();

}

} // namespace paintdata

namespace shape {

using PointVector = QVector<QPointF>;
using PolygonVector = QVector<PointVector>;

inline void tognuplot(const PolygonVector& pps, quint32 index, bool onefile) {

  QString gpath;
  if (onefile) {
    gpath = QString("shape.gnuplot");
  } else {
    gpath = QString("shape_rec_%1.gnuplot").arg(index);
  }

  QFile file(gpath);
  if (onefile) {
    file.open(QFile::ReadWrite);
  } else {
    file.open(QFile::WriteOnly);
  }

  QTextStream stream(&file);
  if (onefile) {
    stream.seek(file.size());
  }

  stream << "\n";

  quint8 color = 0;
  for (const PointVector& ps: pps) {
    for (const QPointF& p: ps) {
      stream << qSetRealNumberPrecision(20) << p.x() << " " << p.y() << " " << color << "\n";
    }
    stream << qSetRealNumberPrecision(20) << ps.first().x() << " " << ps.first().y() << " " << color << "\n";
    stream << "\n";
    color += 1;
  }

  stream << "\n";

}

} // namespace shape

