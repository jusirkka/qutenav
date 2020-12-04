#include "s52presentation_p.h"
#include <QDebug>

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

  for (; i != p->lookupTable[t].end() && i.key() == code; ++i) {
    S52::Lookup* lup = p->lookups[i.value()];
    if (lup->attributes().isEmpty()) {
      index = i.value(); // the default
      continue;
    }
    if (lup->attributes().size() != obj->attributes().size()) continue;
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
      index = i.value();
      break;
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
