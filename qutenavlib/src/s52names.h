/* -*- coding: utf-8-unix -*-
 *
 * File: src/s52names.h
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

#include "types.h"

namespace S52 {

void InitNames();

using AttributeIndexVector = QVector<quint32>;

QString GetClassDescription(quint32 index);
QString GetClassInfo(quint32 code);

const AttributeIndexVector& GetCategoryA(quint32 code);
const AttributeIndexVector& GetCategoryB(quint32 code);
const AttributeIndexVector& GetCategoryC(quint32 code);

S57::AttributeType GetAttributeType(quint32 index);
QString GetAttributeName(quint32 index);

QString GetAttributeDescription(quint32 index);
QString GetAttributeValueDescription(quint32 index, const QVariant& value);
QString GetAttributeEnumDescription(quint32 index, quint32 enumIndex);

quint32 FindCIndex(const QString& name);
quint32 FindCIndex(const QString& name, bool* ok);
QString FindPath(const QString& filename);
bool IsMetaClass(quint32 code);

}
