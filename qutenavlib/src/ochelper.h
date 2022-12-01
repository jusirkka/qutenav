/* -*- coding: utf-8-unix -*-
 *
 * File: src/ochelper.h
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

#include <QString>
#include <QByteArray>

class OCHelper {
public:

  enum class ReadMode {ReadSENC, ReadHeader};

  virtual QString getChartKey(const QString& path) const = 0;
  virtual char getCommand(ReadMode mode) const = 0;
  virtual QByteArray encodeMsg(ReadMode mode,
                               const QString& path,
                               const QString& endPoint,
                               const QString& chartkey) const = 0;

  virtual ~OCHelper() = default;

};

namespace OCHelperNS {
template<typename T> QByteArray encodeMsg(const OCHelper* helper,
                                          OCHelper::ReadMode mode,
                                          const QString& path,
                                          const QString& endPoint,
                                          const QString& chartKey) {
  T msg;
  msg.cmd = helper->getCommand(mode);
  strncpy(msg.fifo_name, endPoint.toUtf8().constData(), sizeof(msg.fifo_name));
  strncpy(msg.senc_name, path.toUtf8().constData(), sizeof(msg.senc_name));
  strncpy(msg.senc_key, chartKey.toUtf8().constData(), sizeof(msg.senc_key));

  QByteArray res;
  res.append(msg.cmd);
  res.append(msg.fifo_name, sizeof(msg.fifo_name));
  res.append(msg.senc_name, sizeof(msg.senc_name));
  res.append(msg.senc_key, sizeof(msg.senc_key));
  return res;
}

}
