/* -*- coding: utf-8-unix -*-
 *
 * oedevice.cpp
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

#include "oedevice.h"

#include "logging.h"
#include <QFileInfo>
#include <QDir>
#include "types.h"
#include <QApplication>
#include <QCryptographicHash>
#include <QProcess>

extern "C" {
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
}

OeDevice::OeDevice(const QString& path, ReadMode mode)
  : QIODevice()
  , m_mode(mode)
  , m_path(path)
  , m_userKey()
  , m_clientEP(-1)
{

  if (m_path.size() > 255) {
    throw ChartFileError(QString("Chart path %1 longer than 255 bytes").arg(m_path));
  }

  // Get the user key
  QFileInfo info(m_path);
  QDir charts(info.path());
  if (!charts.exists(QStringLiteral("Chartinfo.txt"))) {
    throw ChartFileError(QString("Chartinfo.txt not found in %1").arg(info.path()));
  }
  QFile afile(charts.filePath(QStringLiteral("Chartinfo.txt")));
  afile.open(QFile::ReadOnly);
  QTextStream stream(&afile);
  while (!stream.atEnd()) {
    const QStringList parts = stream.readLine().split(":");
    if (parts.length() != 2) continue;
    if (parts[0] == QStringLiteral("UserKey")) {
      m_userKey = parts[1].trimmed();
      break;
    }
  }
  afile.close();
  if (m_userKey.isEmpty()) {
    throw ChartFileError(QStringLiteral("User key not found in Chartinfo.txt"));
  }
  if (m_userKey.size() > 255) {
    throw ChartFileError(QString("User key %1 longer than 255 bytes").arg(m_userKey));
  }

  // Create client endpoint
  m_clientEPName = QString("%1/%2-%3-%4-%5")
      .arg(QDir::tempPath())
      .arg(QApplication::applicationName())
      .arg(QApplication::applicationPid())
      .arg(static_cast<uint>(m_mode))
      .arg(QString(CacheId(m_path)));


  if (m_clientEPName.size() > 255) {
    throw ChartFileError(QString("Client endpoint name %1 longer than 255 bytes").arg(m_clientEPName));
  }

  QFileInfo tmp(m_clientEPName);
  if (tmp.exists()) {
    qCDebug(CENC) << "removing" << m_clientEPName;
    QDir::temp().remove(m_clientEPName);
  }
  if (mkfifo(m_clientEPName.toUtf8().constData(), S_IRUSR | S_IWUSR) < 0) {
    qCWarning(CENC) << strerror(errno);
    throw ChartFileError(QString("Failed to create %1").arg(m_clientEPName));
  }
}

OeDevice::~OeDevice() {
  qCDebug(CENC) << "~OeDevice";
  close();
}

bool OeDevice::atEnd() const {
  // qCDebug(CENC) << "OeDevice::atEnd" << (m_clientEP < 0);
  return m_clientEP < 0;
}

qint64 OeDevice::bytesAvailable() const {
  qCDebug(CENC) << "OeDevice::bytesAvailable" << QIODevice::bytesAvailable();
  return QIODevice::bytesAvailable();
}

void OeDevice::close() {
  qCDebug(CENC) << "OeDevice::close";
  if (m_clientEP > 0) {
    ::close(m_clientEP);
  }
  QFileInfo tmp(m_clientEPName);
  if (tmp.exists()) {
    qCDebug(CENC) << "removing" << m_clientEPName;
    QDir::temp().remove(m_clientEPName);
  }
}

bool OeDevice::isSequential() const {
  return true;
}

bool OeDevice::open(OpenMode mode) {
  if (!(mode & ReadOnly)) {
    qCDebug(CENC) << "OeDevice::open failed: not readonly";
    return false;
  }

  if (!QIODevice::open(mode | Unbuffered)) {
    qCDebug(CENC) << "OeDevice::open failed";
    return false;
  }

  auto serverEP = ::open(serverEPName, O_WRONLY | O_NDELAY);
  if (serverEP < 0) {
    qCWarning(CENC) << strerror(errno);
    return false;
  }


  FifoMessage msg;
  msg.cmd = static_cast<char>(m_mode);
  strncpy(msg.fifo_name, m_clientEPName.toUtf8().constData(), 256);
  strncpy(msg.senc_name, m_path.toUtf8().constData(), 256);
  strncpy(msg.senc_key, m_userKey.toUtf8().constData(), 256);

  ::write(serverEP, reinterpret_cast<char*>(&msg), sizeof(msg));

  ::close(serverEP);

  m_clientEP = ::open(m_clientEPName.toUtf8().constData(), O_RDONLY);
  if (m_clientEP < 0) {
    qCWarning(CENC) << strerror(errno);
    return false;
  }
  return true;
}

#define READ_SIZE 64000LL
#define MAX_TRIES 100

qint64 OeDevice::readData(char* data, qint64 len) {

  if (m_clientEP < 0) return -1;

  qint64 remains = len;
  size_t bytesRead = 0;
  int sleepCnt = MAX_TRIES;

  do {

    size_t bytesToRead = qMin(remains, READ_SIZE);
    const int ret = ::read(m_clientEP, data + bytesRead, bytesToRead);
    if (ret < 0) {
      qCWarning(CENC) << strerror(errno);
      return -1;
    }

    // Server may not have opened the Write end of the FIFO yet
    if (ret == 0) {
      usleep(1000);
      sleepCnt--;
      continue;
    }

    sleepCnt = MAX_TRIES;
    remains -= ret;
    bytesRead += ret;

  } while (remains > 0 && sleepCnt > 0);

  return bytesRead;
}

QByteArray OeDevice::CacheId(const QString& path) {
  QCryptographicHash hash(QCryptographicHash::Sha1);
  hash.addData(path.toUtf8());
  // convert sha1 to base36 form and return first 8 bytes for use as string
  return QByteArray::number(*reinterpret_cast<const quint64*>(hash.result().constData()), 36).left(8);
}

void OeDevice::Kickoff() {
  int cnt = 10;
  auto serverEP = ::open(serverEPName, O_WRONLY | O_NDELAY);
  while ((serverEP < 0) && (cnt > 0)) {
    qCDebug(CENC) << strerror(errno);
    usleep(20000);
    serverEP = ::open(serverEPName, O_WRONLY | O_NDELAY);
    cnt--;
  }
  if (serverEP > 0) {
    qCDebug(CENC) << "oeserverd already running";
    ::close(serverEP);
    return;
  }

  bool ok = QProcess::startDetached(QString(serverName), QStringList());
  qCDebug(CENC) << "launched" << serverName << ", status =" << ok;

  serverEP = ::open(serverEPName, O_WRONLY | O_NDELAY);
  cnt = 20;
  while ((serverEP < 0) && (cnt > 0)) {
    qCDebug(CENC) << strerror(errno);
    usleep(10000);
    serverEP = ::open(serverEPName, O_WRONLY | O_NDELAY);
    cnt--;
  }
  if (serverEP < 0) {
    throw ChartFileError(QString("%1 launched with status %2: cannot open %3")
                         .arg(QString(serverName)).arg(ok).arg(QString(serverEPName)));
  }
  ::close(serverEP);
}
