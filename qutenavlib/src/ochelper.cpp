/* -*- coding: utf-8-unix -*-
 *
 * File: ochelper.cpp
 *
 * Copyright (C) 2023 Jukka Sirkka
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

#include "ochelper.h"
#include <QFileInfo>
#include <QDirIterator>
#include <QXmlStreamReader>
#include <QMap>
#include "types.h"
#include <QTextStream>

class OesuHelper: public OCHelper {
public:
  QString getChartKey(const QString& path) const override {
    // 1. Find all xml files
    QFileInfo info(path);
    const QString base = info.baseName();
    QDirIterator xmls(info.path(), {"*.XML", "*.xml"}, QDir::Files);
    while (xmls.hasNext()) {
      const QString fname = xmls.next();
      // 3. for each xml file: check if it's a keylist file
      QFile file(fname);
      file.open(QFile::ReadOnly);
      QXmlStreamReader reader(&file);
      reader.readNextStartElement();
      if (reader.name() != "keyList") continue;
      // 4. Find <Chart> by <FileName> (basename of path) in <keylist>
      while (reader.readNextStartElement()) {
        if (reader.name() != "Chart") {
          reader.skipCurrentElement();
          continue;
        }
        bool found = false;
        QString key;
        while (reader.readNextStartElement()) {
          if (reader.name() == "FileName") {
            found = reader.readElementText() == base;
          } else if (reader.name() == "RInstallKey") {
            key = reader.readElementText();
          } else {
            reader.skipCurrentElement();
          }
          if (found && !key.isEmpty()) {
            if (key.size() > 512) {
              throw ChartFileError(QString("Chart key %1 is longer than 512 bytes").arg(key));
            }
            // 5. return <RInstallKey>
            return key;
          }
        }
      }
    }

    throw ChartFileError(QString("Chart key not found in Chartkey file %1").arg(info.path()));

    return QString();
  }

  char getCommand(ReadMode mode) const override {
    return readModeMap[mode];
  }

  QByteArray encodeMsg(ReadMode mode, const QString& path, const QString& endPoint, const QString& chartKey) const override {
    return OCHelperNS::encodeMsg<FifoMessage>(this, mode, path, endPoint, chartKey);
  }

  virtual ~OesuHelper() = default;

private:

  static const inline QMap<ReadMode, char> readModeMap =  {
    {ReadMode::ReadHeader, 9},
    {ReadMode::ReadSENC, 8},
  };

  struct FifoMessage {
    char cmd;
    char fifo_name[256];
    char senc_name[256];
    char senc_key[512];
  };

};

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

const OCHelper* OCHelper::getHelper(const QString& path) {
  static QMap<QString, const OCHelper*> helpers {
    {"oesenc", new OesencHelper},
    {"oesu", new OesuHelper}
  };
  const QString ext = QFileInfo(path).suffix();
  if (helpers.contains(ext)) {
    return helpers[ext];
  }
  return nullptr;
}
