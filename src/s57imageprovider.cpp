/* -*- coding: utf-8-unix -*-
 *
 * s57imageprovider.cpp
 *
 * Created: 2021-05-19 2021 by Jukka Sirkka
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
#include "s57imageprovider.h"
#include <QPainter>
#include "chartmanager.h"
#include "logging.h"


S57::ImageProvider::ImageProvider()
  : QQuickImageProvider(QQuickImageProvider::Pixmap)
{}

QPixmap S57::ImageProvider::requestPixmap(const QString& id,
                                          QSize* size, const QSize& requestedSize) {

  const QSize s(PickIconSize, PickIconSize);

  if (size != nullptr) *size = s;

  PickIconData data;
  data.canvas = QPixmap(2 * s);
  data.canvas.fill();

  const auto parts = id.split("/");
  const quint32 chart_id = parts[0].toUInt();
  const auto indexStrings = parts[1].split("-");
  for (const auto& indexString: indexStrings) {
    const quint32 index = indexString.toUInt();
    ChartManager::instance()->paintIcon(data, chart_id, index);
  }
  if (!data.bbox.isValid()) {
    if (size != nullptr) *size = QSize();
    return QPixmap();
  }

  QPixmap target(requestedSize.isValid() ? requestedSize : s);
  target.fill();

  QPainter painter(&target);
  const QPen pen(QColor("black"), 2, Qt::DotLine);
  painter.setPen(pen);
  const int w = target.width();
  const int h = target.height();
  const int x0 = 1;
  const int y0 = 1;
  const int x1 = w - 1;
  const int y1 = h - 1;
  painter.drawLine(x0, y0, x1, y0);
  painter.drawLine(x1, y0, x1, y1);
  painter.drawLine(x1, y1, x0, y1);
  painter.drawLine(x0, y1, x0, y0);

  auto pix = data.canvas.copy(data.bbox.toRect());

  if (pix.size() == target.size()) {
    // areas etc
    painter.drawPixmap(0, 0, pix);
  } else {

    if (pix.width() > PickIconMax * target.width() || pix.height() > PickIconMax * target.height()) {
      if (pix.width() > pix.height()) {
        pix = pix.scaledToWidth(PickIconMax * target.width());
      } else {
        pix = pix.scaledToHeight(PickIconMax * target.height());
      }
    } else if (pix.width() < PickIconMin * target.width() || pix.height() < PickIconMin * target.height()) {
      if (pix.width() > pix.height()) {
        pix = pix.scaledToWidth(PickIconMin * target.width());
      } else {
        pix = pix.scaledToHeight(PickIconMin * target.height());
      }
    }

    QPointF c = .5 * QPointF(target.width() - pix.width(), target.height() - pix.height());
    painter.drawPixmap(c, pix);
  }

  return target;
}
