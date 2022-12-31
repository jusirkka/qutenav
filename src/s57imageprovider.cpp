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
    if (size != nullptr) *size = QSize(1, 1);
    QPixmap dummy(*size);
    dummy.fill();
    return dummy;
  }

  QPixmap target(requestedSize.isValid() ? requestedSize : s);
  target.fill();

  QPainter painter(&target);
  auto pix = data.canvas.copy(data.bbox.toRect());

  if (pix.size() == target.size()) {
    // areas etc
    painter.drawPixmap(0, 0, pix);
  } else {

    const auto mxw = PickIconRelMax * target.width();
    const auto mnw = PickIconRelMin * target.width();
    const auto mxh = PickIconRelMax * target.height();
    const auto mnh = PickIconRelMin * target.height();

    if (pix.width() < mnw && pix.width() >= pix.height()) {
      pix = pix.scaledToWidth(mnw);
    } else if (pix.height() < mnh && pix.height() >= pix.width()) {
      pix = pix.scaledToHeight(mnh);
    } else if (pix.width() > mxw && pix.width() >= pix.height()) {
      pix = pix.scaledToWidth(mxw);
    } else if (pix.height() > mxh && pix.height() >= pix.width()) {
      pix = pix.scaledToHeight(mxh);
    }

    const QPointF c = .5 * QPointF(target.width() - pix.width(), target.height() - pix.height());
    painter.drawPixmap(c, pix);
  }

  return target;
}
