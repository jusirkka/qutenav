/* -*- coding: utf-8-unix -*-
 *
 * File: src/s52presentation.cpp
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
#include "s52presentation_p.h"
#include <QDebug>
#include <QDir>
#include <QStandardPaths>
#include "s52names.h"

S57::PaintDataMap S52::Lookup::execute(const S57::Object *obj) const {

  int stackPos = 0;
  int immedPos = 0;
  int refPos = 0;

  ValueStack stack(20);


  S57::PaintDataMap paintData;

  for (auto code: m_code) {

    switch (code) {

    case Code::Immed:
      stack[stackPos++] = m_immed[immedPos++];
      break;

    case Code::Fun: {
      auto fun = S52::FindFunction(m_references[refPos++]);
      // qDebug() << "function" << fun->name();
      stackPos = 0;
      paintData += fun->execute(stack, obj);
      break;
    }

    case Code::Var:
      stack[stackPos++] = obj->attributeValue(m_references[refPos++]);
      break;

    case Code::DefVar: {
      QVariant v = obj->attributeValue(m_references[refPos++]);
      QVariant d = m_immed[immedPos++];
      if (v.isValid()) {
        stack[stackPos++] = v;
      } else {
        stack[stackPos++] = d;
      }
      break;
    }
    default:
      Q_ASSERT(false);
    }
  }

  return paintData;
}

S52::Lookup* S52::FindLookup(const S57::Object* obj) {
  const quint32 code = obj->classCode();
  const Private::Presentation* p = Private::Presentation::instance();
  const S52::Lookup::Type t = p->typeFilter(obj);

  using LookupVector = QVector<Lookup*>;

  // ordered by attribute count and rcid
  const LookupVector lups = p->lookupTable[t][code];

  for (Lookup* lup: lups) {
    if (lup->attributes().isEmpty()) {
      return lup;
    }
    bool match = true;
    for (auto it = lup->attributes().constBegin(); it != lup->attributes().constEnd(); ++it) {
      if (!obj->attributes().contains(it.key())) {
        match = false;
        break;
      }
      if (!obj->attributes()[it.key()].matches(it.value())) {
        match = false;
        break;
      }
    }
    if (match) {
      return lup;
    }
  }
  // symbology for unknowns
  qDebug() << "No match for" << S52::GetClassInfo(code) << obj->name();
  return p->lookupTable[t][p->names["######"]][0];
}

S52::Function* S52::FindFunction(quint32 index) {
  const Private::Presentation* p = Private::Presentation::instance();
  return p->functions->contents[index];
}

S52::Function* S52::FindFunction(const QString& name) {
  const Private::Presentation* p = Private::Presentation::instance();
  if (!p->functions->names.contains(name)) return nullptr;
  return p->functions->contents[p->functions->names[name]];
}

QColor S52::GetColor(quint32 index) {
  const Private::Presentation* p = Private::Presentation::instance();
  return p->colorTables[p->currentColorTable].colors[index];
}

QColor S52::GetColor(const QString& name) {
  const Private::Presentation* p = Private::Presentation::instance();
  Q_ASSERT(p->names.contains(name));
  return p->colorTables[p->currentColorTable].colors[p->names[name]];
}

QString S52::GetRasterFileName() {
  const Private::Presentation* p = Private::Presentation::instance();
  return S52::FindPath(p->colorTables[p->currentColorTable].graphicsFile);
}

QVariant S52::GetAttribute(const QString &name, const S57::Object *obj) {
  const Private::Presentation* p = Private::Presentation::instance();
  Q_ASSERT(p->names.contains(name));
  return obj->attributeValue(p->names[name]);
}


void S52::InitPresentation() {
  InitNames();
  Private::Presentation* p = Private::Presentation::instance();
  p->init();
}

QString S52::GetSymbolInfo(quint32 index, S52::SymbolType t) {
  const Private::Presentation* p = Private::Presentation::instance();
  const SymbolKey key(index, t);
  if (!p->symbols.contains(key)) return QString();
  return p->symbols[key].code + ": " + p->symbols[key].description;
}


QString S52::GetAttributeInfo(quint32 index, const S57::Object* obj) {
  auto code = GetAttributeName(index);
  if (code.isEmpty()) return QString();

  auto attr = GetAttributeDescription(index);
  QString info;
  info += code + ": " + attr + ": ";
  QVariant v = obj->attributeValue(index);
  switch (GetAttributeType(index)) {
  case S57::Attribute::Type::Real:
    info += QString::number(v.toDouble());
    break;
  case S57::Attribute::Type::String:
    info += v.toString();
    break;
  case S57::Attribute::Type::Integer: {
    auto descr = GetAttributeEnumDescription(index, v.toInt());
    if (!descr.isEmpty()) {
      info += descr;
    } else {
      info += QString::number(v.toInt());
    }
    break;
  }
  case S57::Attribute::Type::IntegerList:
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

quint32 S52::FindIndex(const QString &name) {
  const Private::Presentation* p = Private::Presentation::instance();
  Q_ASSERT(p->names.contains(name));
  return p->names[name];
}

quint32 S52::FindIndex(const QString &name, bool* ok) {
  const Private::Presentation* p = Private::Presentation::instance();
  if (p->names.contains(name)) {
    if (ok != nullptr) *ok = true;
    return p->names[name];
  }
  if (ok != nullptr) *ok = false;
  return 0;
}

