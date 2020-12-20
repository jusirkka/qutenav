#include "s52presentation_p.h"
#include <QDebug>
#include <QDir>
#include <QStandardPaths>

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

void S52::InitPresentation() {
  Private::Presentation* p = Private::Presentation::instance();
  p->init();
}

QString S52::FindPath(const QString& s) {
  QDir dataDir;
  QStringList locs;
  QString file;

  for (const QString& loc: QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation)) {
    locs << QString("%1/qopencpn/s57data").arg(loc);
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

QString S52::GetSymbolInfo(quint32 index, S52::SymbolType t) {
  const Private::Presentation* p = Private::Presentation::instance();
  const SymbolKey key(index, t);
  if (!p->symbols.contains(key)) return QString();
  return p->symbols[key].code + ": " + p->symbols[key].description;
}

QString S52::GetAttributeInfo(quint32 index, const S57::Object* obj) {
  const Private::Presentation* p = Private::Presentation::instance();
  if (!p->attributes.contains(index)) return QString();
  QString info;
  auto attr = p->attributes[index];
  info += attr.code + ": " + attr.description + ": ";
  QVariant v = obj->attributeValue(index);
  switch (attr.type) {
  case S57::Attribute::Type::Real:
    info += QString::number(v.toDouble());
    break;
  case S57::Attribute::Type::String:
    info += v.toString();
    break;
  case S57::Attribute::Type::Integer:
    if (!attr.enumDescriptions.isEmpty()) {
      info += attr.enumDescriptions[v.toInt()];
    } else {
      info += QString::number(v.toInt());
    }
    break;
  case S57::Attribute::Type::IntegerList:
  {
    auto items = v.toList();
    for (auto a: items) {
      if (!attr.enumDescriptions.isEmpty()) {
        info += attr.enumDescriptions[a.toInt()] + ", ";
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


QString S52::GetClassInfo(quint32 index) {
  const Private::Presentation* p = Private::Presentation::instance();
  if (!p->classes.contains(index)) return QString();
  auto cl = p->classes[index];
  return cl.code + ": " + cl.description;
}

S57::Attribute::Type S52::GetAttributeType(quint32 index) {
  const Private::Presentation* p = Private::Presentation::instance();
  if (!p->attributes.contains(index)) return S57::Attribute::Type::None;
  return p->attributes[index].type;
}

QString S52::GetAttributeName(quint32 index) {
  const Private::Presentation* p = Private::Presentation::instance();
  if (!p->attributes.contains(index)) return QString();
  return p->attributes[index].code;
}
