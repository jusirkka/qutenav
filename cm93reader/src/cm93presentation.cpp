/* -*- coding: utf-8-unix -*-
 *
 * File: cm93reader/src/cm93presentation.cpp
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
#include <QHash>
#include "cm93presentation.h"
#include <QFile>
#include <QTextStream>
#include <QStringList>
#include <QDebug>
#include <QStandardPaths>
#include <QDir>

class Presentation {

public:

  static Presentation* instance();

  Presentation() = default;

  void readObjectClasses(const QString& path);
  void readAttributes(const QString& path);

  using IdentifierHash = QHash<QString, quint32>;

  struct ClassDescription {
    ClassDescription(const QString& c, const QString& d, char g, bool meta)
      : code(c)
      , description(d)
      , geom(g)
      , isMeta(meta) {}

    ClassDescription() = default;

    QString code;
    QString description;
    char geom;
    bool isMeta;
  };

  using ClassHash = QHash<quint32, ClassDescription>;

  IdentifierHash names;
  ClassHash classes;

  struct AttributeDescription {
    AttributeDescription(const QString& c, CM93::DataType t)
      : code(c)
      , type(t) {}

    AttributeDescription() = default;

    QString code;
    CM93::DataType type;
  };
  using AttributeMap = QMap<quint32, AttributeDescription>;

  AttributeMap attributes;

};

Presentation* Presentation::instance() {
  static Presentation* p = new Presentation();
  return p;
}

void Presentation::readObjectClasses(const QString &path) {

  QFile file(path);
  file.open(QFile::ReadOnly);
  QTextStream s(&file);

  const QVector<char> geoms{'P', 'L', 'A', 'C', 'S'};

  while (!s.atEnd()) {
    const QStringList parts = s.readLine().split("|");
    // qDebug() << parts;
    if (parts.length() < 5) continue;
    const QString className = parts[0];
    bool ok;
    const quint32 classCode = parts[1].toUInt(&ok);
    if (!ok) continue;
    const char geom = parts[2].toUtf8().constData()[0];
    if (!geoms.contains(geom)) continue;
    bool meta = parts[3] == "0";
    const QString description = parts[4];
    names[className] = classCode;
    classes[classCode] = ClassDescription(className, description, geom, meta);
  }

  file.close();

}

void Presentation::readAttributes(const QString &path) {

  QFile file(path);
  file.open(QFile::ReadOnly);
  QTextStream s(&file);

  const QMap<QString, CM93::DataType> datatypes
  {
    {"aSTRING", CM93::DataType::String},
    {"aBYTE", CM93::DataType::Byte},
    {"aWORD10", CM93::DataType::Word},
    {"aLIST", CM93::DataType::List},
    {"aNotUsed", CM93::DataType::Any},
    {"aFLOAT", CM93::DataType::Float},
    {"aCMPLX", CM93::DataType::Text},
    {"aLONG", CM93::DataType::Long},
  };

  while (!s.atEnd()) {
    const QStringList parts = s.readLine().split("|");
    // qDebug() << parts;
    if (parts.length() < 3) continue;
    const QString attrName = parts[0];
    bool ok;
    const quint32 attrCode = parts[1].toUInt(&ok);
    if (!ok) continue;
    const QString tname = parts[2];
    if (!datatypes.contains(tname)) continue;
    const CM93::DataType t = datatypes[tname];
    names[attrName] = attrCode;
    attributes[attrCode] = AttributeDescription(attrName, t);
  }

  file.close();

}

static QString FindPath(const QString& s) {
  QDir dataDir;
  QStringList locs;
  QString file;

  // qutenav or harbour-qutenav
  const QString baseapp = qAppName().split("_").first();
  for (const QString& loc: QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation)) {
    locs << QString("%1/%2/charts/cm93").arg(loc).arg(baseapp);
  }

  for (const QString& loc: locs) {
    dataDir = QDir(loc);
    const QStringList files = dataDir.entryList(QStringList() << s,
                                                QDir::Files | QDir::Readable);
    if (files.size() == 1) {
      file = files.first();
      break;
    }
  }

  if (file.isEmpty()) {
    throw ChartFileError(QString("%1 not found in any of the standard locations").arg(s));
  }

  return dataDir.absoluteFilePath(file);
}


void CM93::InitPresentation() {
  auto p = Presentation::instance();
  p->readObjectClasses(FindPath("CM93OBJ.DIC"));
  p->readAttributes(FindPath("CM93ATTR.DIC"));
}


CM93::DataType CM93::GetAttributeType(quint32 index) {
  const Presentation* p = Presentation::instance();
  if (!p->attributes.contains(index)) return DataType::None;
  return p->attributes[index].type;
}

QString CM93::GetAttributeName(quint32 index) {
  const Presentation* p = Presentation::instance();
  if (!p->attributes.contains(index)) return QString();
  return p->attributes[index].code;
}


quint32 CM93::FindIndex(const QString &name) {
  const Presentation* p = Presentation::instance();
  Q_ASSERT(p->names.contains(name));
  return p->names[name];
}

quint32 CM93::FindIndex(const QString &name, bool* ok) {
  const Presentation* p = Presentation::instance();
  if (p->names.contains(name)) {
    if (ok != nullptr) *ok = true;
    return p->names[name];
  }
  if (ok != nullptr) *ok = false;
  return 0;
}


QString CM93::GetClassInfo(quint32 code) {
  const Presentation* p = Presentation::instance();
  if (!p->classes.contains(code)) return QString();
  auto cl = p->classes[code];
  return cl.code + ": " + cl.description;
}

QString CM93::GetClassName(quint32 code) {
  const Presentation* p = Presentation::instance();
  if (!p->classes.contains(code)) return QString();
  return p->classes[code].code;
}

bool CM93::IsMetaClass(quint32 code) {
  const Presentation* p = Presentation::instance();
  Q_ASSERT(p->classes.contains(code));
  return p->classes[code].isMeta;
}

