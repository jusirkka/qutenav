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


S57::ImageProvider::ImageProvider()
  : QQuickImageProvider(QQuickImageProvider::Pixmap)
{}

QPixmap S57::ImageProvider::requestPixmap(const QString& id,
                                          QSize* size, const QSize& requestedSize) {

  const QSize s(100, 100);
  if (size) {
    *size = s;
  }
  QPixmap pixmap(requestedSize.isValid() ? requestedSize : s);
  pixmap.fill();
  QPainter painter(&pixmap);

  const auto parts = id.split("/");
  const quint32 chart_id = parts[0].toUInt();
  const auto indexStrings = parts[1].split("-");
  for (const auto& indexString: indexStrings) {
    const quint32 index = indexString.toUInt();
    ChartManager::instance()->paintIcon(painter, chart_id, index);
  }
  return pixmap;
}



