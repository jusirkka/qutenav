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
  auto i = p->lookupTable[t].find(code);

  // symbology for unknowns
  auto qcode = p->names["######"];
  auto qi = p->lookupTable[t].find(qcode);
  quint32 index = qi.value();

  // note: the items with the same key in QHash are in reversed order
  // => need to find the last match
  bool match = false;
  int maxMatchedAttributes = 0; // a hack to overcome wrong ordering of lookups
  for (; i != p->lookupTable[t].end() && i.key() == code; ++i) {
    const S52::Lookup* lup = p->lookups[i.value()];
    if (lup->attributes().isEmpty()) {
      if (!match) index = i.value(); // the default
      continue;
    }
    bool amatch = true;
    for (auto it = lup->attributes().constBegin(); it != lup->attributes().constEnd(); ++it) {
      if (!obj->attributes().contains(it.key())) {
        amatch = false;
        break;
      }
      if (!obj->attributes()[it.key()].matches(it.value())) {
        amatch = false;
        break;
      }
    }
    if (amatch && lup->attributes().size() >= maxMatchedAttributes) {
      maxMatchedAttributes = lup->attributes().size();
      index = i.value();
      match = true;
    }
  }
  return p->lookups[index];
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
