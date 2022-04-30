/* -*- coding: utf-8-unix -*-
 *
 * utils.cpp
 *
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

#include <QStandardPaths>
#include "logging.h"
#include "platform.h"
#include "settings.h"
#include <QDir>
#include <QDateTime>


void checkCache(bool clearCache) {
  const auto base = QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation);
  const auto path = QString("%1/%2").arg(base).arg(baseAppName());
  quint32 mx = Settings::instance()->cacheSize() * 1000000; // megabytes -> bytes
  if (clearCache) {
    mx = 0;
  }

  QDir cacheDir(path);
  if (!cacheDir.exists()) return;
  const auto cacheFiles = cacheDir.entryInfoList(QDir::Files, QDir::Time);
  quint32 sz = 0;
  for (const QFileInfo& info: cacheFiles) {
    if (sz < mx) {
      sz += info.size();
      // qDebug() << info.fileName() << info.lastModified() << sz;
    } else {
      if (cacheDir.remove(info.fileName())) {
        qCInfo(CDPY) << "removed" << info.filePath()
                << ", size" << info.size()
                << ", last used" << info.lastModified().toString();
      } else {
        qCWarning(CDPY) << "Cannot remove" << info.filePath();
      }
    }
  }
}

