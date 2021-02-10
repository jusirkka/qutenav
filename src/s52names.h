#pragma once

#include "types.h"

namespace S52 {

void InitNames();

QString GetClassDescription(quint32 index);
QString GetClassInfo(quint32 code);

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
