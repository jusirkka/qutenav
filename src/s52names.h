#pragma once

#include "types.h"

namespace S52 {

void InitNames();

S57::AttributeType GetAttributeType(quint32 index);
QString GetAttributeName(quint32 index);
QString GetAttributeDescription(quint32 index);
QString GetAttributeEnumDescription(quint32 index, quint32 enumIndex);
quint32 FindCIndex(const QString& name);
quint32 FindCIndex(const QString& name, bool* ok);
QString FindPath(const QString& filename);
QString GetClassInfo(quint32 code);
bool IsMetaClass(quint32 code);

}
