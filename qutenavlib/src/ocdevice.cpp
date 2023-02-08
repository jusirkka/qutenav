/* -*- coding: utf-8-unix -*-
 *
 * OCDevice.cpp
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

#include "ocdevice.h"

#include "logging.h"
#include <QFileInfo>
#include <QDir>
#include "types.h"
#include <QApplication>
#include <QCryptographicHash>
#include <QFileSystemWatcher>
#include <QProcess>
#include "platform.h"

extern "C" {
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
}

OCDevice::OCDevice(const QString &path, ReadMode mode, const char* serverEPName)
  : QIODevice()
  , m_mode(mode)
  , m_path(path)
  , m_helper(OCHelper::getHelper(path))
  , m_serverEPName(serverEPName)
{

  setObjectName(path);

  if (m_path.size() > 255) {
    throw ChartFileError(QString("Chart path %1 longer than 255 bytes").arg(m_path));
  }

  m_chartKey = m_helper->getChartKey(m_path);

  // Create client endpoint
  m_clientEPName = QString("%1/%2-%3-%4-%5")
      .arg(QDir::tempPath())
      .arg(QApplication::applicationName())
      .arg(QApplication::applicationPid())
      .arg(static_cast<int>(m_helper->getCommand(m_mode)))
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

OCDevice::~OCDevice() {
  // qCDebug(CENC) << "~OCDevice";
  close();
}

bool OCDevice::atEnd() const {
  if  (m_clientEP < 0) {
    qCDebug(CENC) << "OCDevice::atEnd" << m_clientEPName;
  }
  return m_clientEP < 0;
}

qint64 OCDevice::bytesAvailable() const {
  qCDebug(CENC) << "OCDevice::bytesAvailable" << QIODevice::bytesAvailable();
  return QIODevice::bytesAvailable();
}

void OCDevice::close() {
  // qCDebug(CENC) << "OCDevice::close";
  if (m_clientEP > 0) {
    ::close(m_clientEP);
  }
  QFileInfo tmp(m_clientEPName);
  if (tmp.exists()) {
    // qCDebug(CENC) << "removing" << m_clientEPName;
    QDir::temp().remove(m_clientEPName);
  }
}

bool OCDevice::isSequential() const {
  return true;
}

bool OCDevice::open(OpenMode mode) {
  if (!(mode & ReadOnly)) {
    qCDebug(CENC) << "OCDevice::open failed: not readonly";
    return false;
  }

  if (!QIODevice::open(mode | Unbuffered)) {
    qCDebug(CENC) << "OCDevice::open failed";
    return false;
  }

  auto serverEP = ::open(m_serverEPName.toUtf8().constData(), O_WRONLY | O_NDELAY);
  if (serverEP < 0) {
    qCWarning(CENC) << strerror(errno);
    return false;
  }

  auto bytes = m_helper->encodeMsg(m_mode, m_path, m_clientEPName, m_chartKey);

  ::write(serverEP, bytes.constData(), bytes.size());

  ::close(serverEP);

  m_clientEP = ::open(m_clientEPName.toUtf8().constData(), O_RDONLY | O_NONBLOCK);
  if (m_clientEP < 0) {
    qCWarning(CENC) << strerror(errno);
    return false;
  }
  return true;
}

#define READ_SIZE 64000LL
#define MAX_TRIES 100

qint64 OCDevice::readData(char* data, qint64 len) {

  if (m_clientEP < 0) return -1;

  qint64 remains = len;
  size_t bytesRead = 0;
  int sleepCnt = MAX_TRIES;

  do {

    size_t bytesToRead = qMin(remains, READ_SIZE);
    const int ret = ::read(m_clientEP, data + bytesRead, bytesToRead);
    if (ret < 0 && errno != EAGAIN) {
      qCWarning(CENC) << strerror(errno);
      return -1;
    }

    if (ret <= 0) {
      usleep(1000);
      sleepCnt--;
      continue;
    }

    sleepCnt = MAX_TRIES;
    remains -= ret;
    bytesRead += ret;

  } while (remains > 0 && sleepCnt > 0);

  if (sleepCnt <= 0) {
    return -1;
  }

  return bytesRead;
}

QByteArray OCDevice::CacheId(const QString& path) {
  QCryptographicHash hash(QCryptographicHash::Sha1);
  hash.addData(path.toUtf8());
  // convert sha1 to base36 form and return first 8 bytes for use as string
  return QByteArray::number(*reinterpret_cast<const quint64*>(hash.result().constData()), 36).left(8);
}

int OCServerManager::checkServer() const {

  QProcess pidof;
  pidof.setProgram("pidof");
  pidof.setArguments(QStringList() << m_serverPath);

  pidof.start(QIODevice::ReadOnly);
  pidof.waitForFinished();
  bool up;
  int pid = pidof.readAll().trimmed().toInt(&up);
  //  if (up) {
  //    qCDebug(CENC) << m_serverPath << "is running";
  //  }

  const QFileInfo ep(m_serverEP);
  if (up && ep.exists()) {
    // qCDebug(CENC) << m_serverEP << "exists";
    return pid;
  }

  for (const QString& dir: m_watcher->directories()) {
    qCDebug(CENC) << "unwatching" << dir;
    m_watcher->removePath(dir);
  }
  for (const QString& file: m_watcher->files()) {
    qCDebug(CENC) << "unwatching" << file;
    m_watcher->removePath(file);
  }

  QDir(ep.absolutePath()).remove(ep.fileName());

  QProcess kill;
  kill.start("killall", QStringList() << m_serverPath);
  kill.waitForFinished();

  qCDebug(CENC) << "starting" << m_serverPath;
  QProcess server;
  server.start(m_serverPath);
  server.waitForFinished();

  pidof.start(QIODevice::ReadOnly);
  pidof.waitForFinished();
  pid = pidof.readAll().trimmed().toInt(&up);
  if (!up) {
    throw ChartFileError(QString("Cannot start %1").arg(m_serverPath));
  }

  int cnt = 100;
  while (!ep.exists() && cnt > 0) {
    qCDebug(CENC) << "Waiting for" << m_serverPath << "to create" << m_serverEP << "...";
    usleep(20000);
    cnt--;
  }
  if (cnt <= 0) {
    throw ChartFileError(QString("%1 didn't create %2").arg(m_serverPath).arg(m_serverEP));
  }
  qCDebug(CENC) << m_serverPath << "is in service";

  return pid;
}

OCServerManager::OCServerManager(const QString& serverPath, const QString& serverEP)
  : QObject()
  , m_watcher(new QFileSystemWatcher(this))
  , m_serverPath(serverPath)
  , m_serverEP(serverEP)
{
  serverRestart("initial");
  if (Platform::base_app_name() == qAppName()) { // do not watch in dbupdater
    connect(m_watcher, &QFileSystemWatcher::fileChanged, this, &OCServerManager::serverRestart);
    connect(m_watcher, &QFileSystemWatcher::directoryChanged, this, &OCServerManager::serverRestart);
  }
}

void OCServerManager::serverRestart(const QString& path) {
  qCDebug(CENC) << "changed:" << path;
  int pid = checkServer();
  if (Platform::base_app_name() == qAppName()) { // do not watch in dbupdater
    const QString procdir = QString("/proc/%1").arg(pid);
    m_watcher->addPath(procdir);
    m_watcher->addPath(m_serverEP);
    qCDebug(CENC) << "Watching" << m_watcher->files() << m_watcher->directories();
  }
}
