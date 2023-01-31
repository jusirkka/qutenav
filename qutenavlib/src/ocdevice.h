/* -*- coding: utf-8-unix -*-
 *
 * ocdevice.h
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
#include "ochelper.h"

using ReadMode = OCHelper::ReadMode;

class QFileSystemWatcher;

class OCDevice: public QIODevice {

  Q_OBJECT

public:

  OCDevice(const QString& path, ReadMode mode, const char* serverEPName);
  virtual ~OCDevice();

  bool atEnd() const override;
  qint64 bytesAvailable() const override;
  void close() override;
  bool isSequential() const override;
  bool open(OpenMode mode) override;

  static void Kickoff(const char* serverPath, const char* serverEPName);

protected:

  qint64 readData(char* data, qint64 maxlen) override;
  qint64 writeData(const char*, qint64) override {return -1;} // readonly device

private:

  static QByteArray CacheId(const QString& path);

  ReadMode m_mode;
  QString m_path;
  QString m_chartKey;
  const OCHelper* m_helper;

  const QString m_serverEPName;

  QString m_clientEPName;
  int m_clientEP = -1;
};

class OCServerManager: public QObject {
  Q_OBJECT

public:

  OCServerManager(const QString& serverPath, const QString& serverEP);

  void init() {/* dummy */}

private slots:

  void serverRestart(const QString& path);

private:

  int checkServer() const;

  QFileSystemWatcher* m_watcher;
  const QString m_serverPath;
  const QString m_serverEP;
};


