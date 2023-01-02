/* -*- coding: utf-8-unix -*-
 *
 * File: oesencreader/src/oesencreader.cpp
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
#include "oesencreader.h"
#include "osenc.h"
#include "ocdevice.h"
#include <QFileInfo>
#include <QDir>
#include <QTextStream>

class OesencHelper: public OCHelper {
public:
  QString getChartKey(const QString& path) const override {
    QFileInfo info(path);
    QDir charts(info.path());
    if (!charts.exists(QStringLiteral("Chartinfo.txt"))) {
      throw ChartFileError(QString("Chartinfo.txt not found in %1").arg(info.path()));
    }
    QFile afile(charts.filePath(QStringLiteral("Chartinfo.txt")));
    afile.open(QFile::ReadOnly);
    QTextStream stream(&afile);
    QString userKey;
    while (!stream.atEnd()) {
      const QStringList parts = stream.readLine().split(":");
      if (parts.length() != 2) continue;
      if (parts[0] == QStringLiteral("UserKey")) {
        userKey = parts[1].trimmed();
        break;
      }
    }
    afile.close();
    if (userKey.isEmpty()) {
      throw ChartFileError(QStringLiteral("User key not found in Chartinfo.txt"));
    }
    if (userKey.size() > 255) {
      throw ChartFileError(QString("User key %1 longer than 255 bytes").arg(userKey));
    }

    return userKey;
  }

  char getCommand(ReadMode mode) const override {
    return readModeMap[mode];
  }

  QByteArray encodeMsg(ReadMode mode, const QString& path, const QString& endPoint, const QString& chartKey) const override {
    return OCHelperNS::encodeMsg<FifoMessage>(this, mode, path, endPoint, chartKey);
  }

  virtual ~OesencHelper() = default;

private:

  static const inline QMap<ReadMode, char> readModeMap =  {
    {ReadMode::ReadHeader, 3},
    {ReadMode::ReadSENC, 0},
  };

  struct FifoMessage {
    char cmd;
    char fifo_name[256];
    char senc_name[256];
    char senc_key[256];
  };

};

class OesencDevice: public OCDevice {
public:
  OesencDevice(const QString& path, ReadMode mode):
    OCDevice(path, mode, new OesencHelper, serverEPName) {}

  static inline const char serverEPName[] = "/tmp/OCPN_PIPE";
  static inline const char serverPath[] = "/usr/bin/oeserverd";

};


const GeoProjection* OesencReader::geoprojection() const {
  return m_proj;
}

OesencReader::OesencReader(const QString& name)
  : ChartFileReader(name)
  , m_proj(GeoProjection::CreateProjection("SimpleMercator"))
{}

OesencReader::~OesencReader() {
  delete m_proj;
}


GeoProjection* OesencReader::configuredProjection(const QString &path) const {

  OesencDevice device(path, ReadMode::ReadHeader);
  if (!device.open(OCDevice::ReadOnly)) {
    throw ChartFileError(QString("Cannot open %1 for reading").arg(path));
  }

  Osenc senc;
  return senc.configuredProjection(&device, m_proj->className());
}

S57ChartOutline OesencReader::readOutline(const QString &path, const GeoProjection *gp) const {

  OesencDevice device(path, ReadMode::ReadHeader);
  if (!device.open(OCDevice::ReadOnly)) {
    throw ChartFileError(QString("Cannot open %1 for reading").arg(path));
  }

  Osenc senc;
  return senc.readOutline(&device, gp);

}


void OesencReader::readChart(GL::VertexVector& vertices,
                            GL::IndexVector& indices,
                            S57::ObjectVector& objects,
                            const QString& path,
                            const GeoProjection* gp) const {
  OesencDevice device(path, ReadMode::ReadSENC);
  if (!device.open(OCDevice::ReadOnly)) {
    throw ChartFileError(QString("Cannot open %1 for reading").arg(path));
  }

  Osenc senc;
  senc.readChart(vertices, indices, objects, &device, gp);
}


QString OesencReaderFactory::name() const {
  return "oesenc";
}

QString OesencReaderFactory::displayName() const {
  return "OSENC Encrypted Charts";
}

QStringList OesencReaderFactory::filters() const {
  return QStringList {"*.oesenc"};
}

void OesencReaderFactory::initialize(const QStringList&) const {
  OCDevice::Kickoff(OesencDevice::serverPath, OesencDevice::serverEPName);
}

ChartFileReader* OesencReaderFactory::create() const {
  return new OesencReader(name());
}
