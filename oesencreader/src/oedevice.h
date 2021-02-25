/* -*- coding: utf-8-unix -*-
 *
 * oedevice.h
 *
 * Created: 2021-02-23 2021 by Jukka Sirkka
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

#include <QIODevice>

class OeDevice: public QIODevice {

  Q_OBJECT

public:

  enum ReadMode {ReadSENC = 0, ReadHeader = 3};

  OeDevice(const QString& path, ReadMode mode);
  virtual ~OeDevice();

  bool atEnd() const override;
  qint64 bytesAvailable() const override;
  void close() override;
  bool isSequential() const override;
  bool open(OpenMode mode) override;

  static void Kickoff();

protected:

  qint64 readData(char* data, qint64 maxlen) override;
  qint64 writeData(const char*, qint64) override {return -1;} // readonly device

private:

  struct FifoMessage {
    char cmd;
    char fifo_name[256];
    char senc_name[256];
    char senc_key[256];
  };

  static const qint8 CmdExit = 2;
  static const qint8 CmdTestAvail = 1;

  static inline const char serverEPName[] = "/tmp/OCPN_PIPE";
  static inline const char serverName[] = "/usr/bin/oeserverd";

  static QByteArray CacheId(const QString& path);

  ReadMode m_mode;
  QString m_path;
  QString m_userKey;
  QString m_clientEPName;
  int m_clientEP;
};


