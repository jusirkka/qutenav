/* -*- coding: utf-8-unix -*-
 *
 * s57imageprovider.h
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
#pragma once

#include <QQuickImageProvider>

namespace S57 {

class ImageProvider: public QQuickImageProvider {
public:

  ImageProvider();
  QPixmap requestPixmap(const QString& id, QSize* size, const QSize& requestedSize) override;

private:

  static const inline double PickIconRelMin = .60;
  static const inline double PickIconRelMax = .90;

};

} // namespace S57

