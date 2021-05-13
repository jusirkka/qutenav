/* -*- coding: utf-8-unix -*-
 *
 * File: cm93reader/src/cm93presentation.h
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
#include "types.h"

namespace CM93 {

enum class DataType: char {
  String = 'S',
  Byte = 'B',
  List = 'L',
  Word = 'W',
  Float = 'R',
  Long = 'G',
  Text = 'C',
  Any = '?',
  None = '0'
};

static const inline QVector<char> AllDataTypes {'S', 'B', 'L', 'W', 'R', 'G', 'C', '?', '0'};



enum class GeomType: quint8 {
  None = 0,
  Point = 1,
  Line = 2,
  Area = 4,
  Sounding = 8
};

static const inline QVector<quint8> AllGeomTypes {0, 1, 2, 4, 8};

void InitPresentation(const QStringList& paths);

QString GetAttributeName(quint32 index);
DataType GetAttributeType(quint32 index);
quint32 FindIndex(const QString& name);
quint32 FindIndex(const QString& name, bool* ok);
QString GetClassInfo(quint32 code);
QString GetClassName(quint32 code);
bool IsMetaClass(quint32 code);


}

