/* -*- coding: utf-8-unix -*-
 *
 * File: src/s52names.cpp
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
#include "s52names.h"
#include <QStandardPaths>
#include <QDir>
#include <QTextStream>
#include <QDebug>
#include <QRegularExpression>

namespace S52 {

class Names {

public:

  static Names* instance();

private:

  Names();

  void readObjectClasses();
  void readAttributes();

public:

  void init();

  using IdentifierHash = QHash<QString, quint32>;

  struct ClassDescription {
    ClassDescription(const QString& c, const QString& d, bool meta)
      : code(c)
      , description(d)
      , isMeta(meta) {}

    ClassDescription() = default;

    QString code;
    QString description;
    bool isMeta;
  };

  using ClassHash = QHash<quint32, ClassDescription>;

  IdentifierHash names;
  ClassHash classes;

  using DescriptionMap = QMap<quint32, QString>;

  struct AttributeDescription {
    AttributeDescription(const QString& c, S57::AttributeType t, const QString& d)
      : code(c)
      , type(t)
      , description(d) {}

    AttributeDescription() = default;

    QString code;
    S57::AttributeType type;
    QString description;
    DescriptionMap enumDescriptions;
  };
  using AttributeMap = QMap<quint32, AttributeDescription>;

  AttributeMap attributes;
};

}

S52::Names* S52::Names::instance() {
  static Names* names = new Names();
  return names;
}

S52::Names::Names() {
  readAttributes();
  readObjectClasses();
}

void S52::Names::init() {
  // noop
}


void S52::Names::readObjectClasses() {
  QFile file(S52::FindPath("s57objectclasses.csv"));
  file.open(QFile::ReadOnly);
  QTextStream s(&file);

  while (!s.atEnd()) {
    const QStringList parts = s.readLine().split(",");
    bool ok;
    const quint32 classCode = parts[0].toUInt(&ok);
    if (!ok) continue;
    QString description = parts[1];
    int i = 1;
    while (!description.endsWith("\"")) {
      i += 1;
      description += ",";
      description += parts[i];
    }
    description.replace("\"", "");
    const QString className = parts[i + 1];
    if (names.contains(className)) {
      qWarning() << className << "already parsed";
      continue;
    }
    bool meta = parts[parts.length() - 2] == "M";

    names[className] = classCode;
    classes[classCode] = ClassDescription(className, description, meta);

  }

  // Object class for unknowns
  const quint32 unknownCode = 666666;
  const QString unknownClass("######");
  names[unknownClass] = unknownCode;
  classes[unknownCode] = ClassDescription(unknownClass, "Unknown object class", false);

  file.close();
}


void S52::Names::readAttributes() {
  const QMap<QString, S57::AttributeType> typeLookup {
    {"A", S57::AttributeType::String},
    {"I", S57::AttributeType::Integer},
    {"S", S57::AttributeType::String},
    {"E", S57::AttributeType::Integer},
    {"L", S57::AttributeType::IntegerList},
    {"F", S57::AttributeType::Real},
  };

  QFile afile(S52::FindPath("s57attributes.csv"));
  afile.open(QFile::ReadOnly);
  QTextStream s1(&afile);

  while (!s1.atEnd()) {
    const QStringList parts = s1.readLine().split(",");
    if (parts.length() != 5) continue;
    bool ok;
    const quint32 id = parts[0].toUInt(&ok);
    if (!ok) continue;
    const QString description = parts[1];
    const QString attributeName = parts[2];
    if (names.contains(attributeName)) {
      qWarning() << attributeName << "already parsed";
      continue;
    }
    const S57::AttributeType t = typeLookup[parts[3]];

    names[attributeName] = id;
    attributes[id] = AttributeDescription(attributeName, t, description);
  }
  afile.close();

  static const QRegularExpression re("^(\\d+),(\\d+),\"(.*)\"$");

  QFile dfile(S52::FindPath("s57expectedinput.csv"));
  dfile.open(QFile::ReadOnly);
  QTextStream s2(&dfile);

  while (!s2.atEnd()) {
    const QString line = s2.readLine();
    const QRegularExpressionMatch match = re.match(line);
    if (!match.hasMatch()) {
      qDebug() << "no description match in line" << line;
      continue;
    }
    const quint32 aid = match.captured(1).toUInt();
    const quint32 eid = match.captured(2).toUInt();
    const QString desc = match.captured(3);

    if (!attributes.contains(aid)) {
      qWarning() << "Attribute" << aid << "not found";
      continue;
    }
    DescriptionMap* target = &attributes[aid].enumDescriptions;
    (*target)[eid] = desc;
  }

  dfile.close();
}

void S52::InitNames() {
  auto p = Names::instance();
  p->init();
}

QString S52::FindPath(const QString& s) {
  QDir dataDir;
  QStringList locs;
  QString file;

  // qutenav or harbour-qutenav
  const QString baseapp = qAppName().split("_").first();
  for (const QString& loc: QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation)) {
    locs << QString("%1/%2/s57data").arg(loc).arg(baseapp);
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
    qFatal("%s not found in any of the standard locations", s.toUtf8().constData());
  }

  return dataDir.absoluteFilePath(file);
}

QString S52::GetClassDescription(quint32 code) {
  auto p = Names::instance();
  if (!p->classes.contains(code)) return QString();
  auto cl = p->classes[code];
  return cl.description;
}

QString S52::GetClassInfo(quint32 code) {
  auto p = Names::instance();
  if (!p->classes.contains(code)) return QString();
  auto cl = p->classes[code];
  return cl.code + ": " + cl.description;
}

S57::AttributeType S52::GetAttributeType(quint32 index) {
  auto p = Names::instance();
  if (!p->attributes.contains(index)) return S57::AttributeType::None;
  return p->attributes[index].type;
}

QString S52::GetAttributeName(quint32 index) {
  auto p = Names::instance();
  if (!p->attributes.contains(index)) return QString();
  return p->attributes[index].code;
}

QString S52::GetAttributeDescription(quint32 index) {
  auto p = Names::instance();
  if (!p->attributes.contains(index)) return QString();
  return p->attributes[index].description;
}

QString S52::GetAttributeValueDescription(quint32 index, const QVariant& v) {
  auto p = Names::instance();
  if (!p->attributes.contains(index)) return QString();

  QString info;
  switch (GetAttributeType(index)) {
  case S57::AttributeType::Real:
    info = QString::number(v.toDouble());
    break;
  case S57::AttributeType::String:
    info = v.toString();
    break;
  case S57::AttributeType::Integer: {
    auto descr = GetAttributeEnumDescription(index, v.toInt());
    if (!descr.isEmpty()) {
      info = descr;
    } else {
      info = QString::number(v.toInt());
    }
    break;
  }
  case S57::AttributeType::IntegerList:
  {
    auto items = v.toList();
    for (auto a: items) {
      auto descr = GetAttributeEnumDescription(index, a.toInt());
      if (!descr.isEmpty()) {
        info += descr + ", ";
      } else {
        info += QString::number(a.toInt()) + ", ";
      }
    }
    if (!items.isEmpty()) info.remove(info.length() - 2, 2);
  }
    break;
  default:
    ; // do nothing
  }
  return info;
}

QString S52::GetAttributeEnumDescription(quint32 aid, quint32 eid) {
  auto p = Names::instance();
  if (!p->attributes.contains(aid)) return QString();
  if (!p->attributes[aid].enumDescriptions.contains(eid)) return QString();
  return p->attributes[aid].enumDescriptions[eid];
}

bool S52::IsMetaClass(quint32 code) {
  auto p = Names::instance();
  Q_ASSERT(p->classes.contains(code));
  return p->classes[code].isMeta;
}

quint32 S52::FindCIndex(const QString &name) {
  auto p = Names::instance();
  Q_ASSERT(p->names.contains(name));
  return p->names[name];
}

quint32 S52::FindCIndex(const QString &name, bool* ok) {
  auto p = Names::instance();
  if (p->names.contains(name)) {
    if (ok != nullptr) *ok = true;
    return p->names[name];
  }
  if (ok != nullptr) *ok = false;
  return 0;
}

