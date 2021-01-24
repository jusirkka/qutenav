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

void InitPresentation();

QString GetAttributeName(quint32 index);
DataType GetAttributeType(quint32 index);
quint32 FindIndex(const QString& name);
quint32 FindIndex(const QString& name, bool* ok);
QString GetClassInfo(quint32 code);
QString GetClassName(quint32 code);
bool IsMetaClass(quint32 code);


}

