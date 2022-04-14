/* -*- coding: utf-8-unix -*-
 *
 * File: src/s52presentation.h
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
#include <QColor>
#include "s57object.h"
#include <QPoint>
#include <QRect>
#include "s52functions.h"
#include "conf_marinerparams.h"


class QPainter;

namespace S52 {

class ByteCoder;

class Lookup {

  friend class ByteCoder;

public:

  enum class Type: char {
    Simplified, // points
    PaperChart, // points
    Lines, // lines
    PlainBoundaries, // areas
    SymbolizedBoundaries, // areas
  };

  static const int PriorityCount = 10;

  using Category = Conf::MarinerParams::EnumMaxCategory::type;

  using AttributeMap = QMap<quint32, S57::Attribute>;

  Lookup(Type t, int id, quint32 code, int prioId, Category cat,
         AttributeMap attrs, QString comment, QString source)
    : m_type(t)
    , m_rcid(id)
    , m_classCode(code)
    , m_priorityId(prioId)
    , m_category(cat)
    , m_attributes(attrs)
    , m_comment(comment)
    , m_source(source)
    , m_needUnderling(false)
    , m_canOverride(false)
  {}

  Type type() const {return m_type;}
  int rcid() const {return m_rcid;}
  quint32 classCode() const {return m_classCode;}
  int priority() const {return m_priorityId;}
  Category category() const {return m_category;}
  const AttributeMap& attributes() const {return m_attributes;}
  const QString& comment() const {return m_comment;}
  const QString& source() const {return m_source;}
  bool byteCodeReady() const {return !m_code.isEmpty();}
  bool canOverride() const {return m_canOverride;}
  bool needUnderling() const {return m_needUnderling;}

  S57::PaintDataMap execute(const S57::Object* obj) const;
  QString description(const S57::Object* obj) const;
  void paintIcon(QPainter& painter, const S57::Object* obj) const;

  // bytecode interface
  enum class Code: quint8 {Immed, Var, Fun, DefVar};

private:

  Type m_type;
  int m_rcid;
  quint32 m_classCode;
  int m_priorityId;
  Category m_category;
  AttributeMap m_attributes;
  QString m_comment;
  QString m_source;

  bool m_needUnderling;
  bool m_canOverride;

  // bytecode interface
  using CodeStack = QVector<Code>;
  using ValueStack = QVector<QVariant>;
  using ReferenceStack = QVector<quint32>;

  CodeStack m_code;
  ValueStack m_immed;
  ReferenceStack m_references;

};

Lookup* FindLookup(const S57::Object* obj);
Function* FindFunction(quint32 index);
Function* FindFunction(const QString& name);
QColor GetColor(quint32 index);
QColor GetColor(const QString& name);
QVariant GetAttribute(const QString& name, const S57::Object* obj);
void InitPresentation();
QString GetRasterFileName();
QString GetSymbolInfo(quint32 index, S52::SymbolType t);
QString GetSymbolInfo(const SymbolKey& key);
QString GetAttributeInfo(quint32 index, const S57::Object* obj);
quint32 FindIndex(const QString& name);
quint32 FindIndex(const QString& name, bool* ok);
void ParseInstruction(Lookup* lup, bool* ok = nullptr);

} // namespace S52
