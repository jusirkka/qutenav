#include "s52presentation_p.h"
#include <QDebug>

S57::PaintDataMap S52::Lookup::execute(const S57::Object *obj) {

  int stackPos = 0;
  int immedPos = 0;
  int refPos = 0;

  S57::PaintDataMap paintData;

  for (auto code: m_code) {

    switch (code) {

    case Code::Immed:
      m_stack[stackPos++] = m_immed[immedPos++];
      break;

    case Code::Fun: {
      auto fun = S52::FindFunction(m_references[refPos++]);
      // qDebug() << "function" << fun->name();
      stackPos = 0;
      const S57::PaintDataMap ps = fun->execute(m_stack, obj);
      for (auto it = ps.constBegin(); it != ps.constEnd(); ++it) {
        if (paintData.contains(it.key())) {
          qWarning() << "Overwriting paint data" << quint8(it.key());
        }
        paintData[it.key()] = it.value();
      }
      break;
    }

    case Code::Var:
      m_stack[stackPos++] = obj->attributes()[m_references[refPos++]].value();
      break;

    default:
      Q_ASSERT(false);
    }
  }

  return paintData;
}

S52::Lookup* S52::FindLookup(const S57::Object* obj) {
  const quint32 code = obj->classCode();
  Private::Presentation* p = Private::Presentation::instance();
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
  // parse the instruction
  int err = p->parseInstruction(index);
  if (err != 0) {
    qWarning() << "Error parsing" << p->lookups[index]->source();
  }
  return p->lookups[index];
}

S52::Function* S52::FindFunction(quint32 index) {
  Private::Presentation* p = Private::Presentation::instance();
  return p->functions.contents[index];
}

S52::Function* S52::FindFunction(const QString& name) {
  Private::Presentation* p = Private::Presentation::instance();
  if (!p->functions.names.contains(name)) return nullptr;
  return p->functions.contents[p->functions.names[name]];
}

QColor S52::GetColor(quint32 index) {
  Private::Presentation* p = Private::Presentation::instance();
  return p->colorTables[p->currentColorTable].colors[index];
}

QColor S52::GetColor(const QString& name) {
  Private::Presentation* p = Private::Presentation::instance();
  Q_ASSERT(p->names.contains(name));
  return p->colorTables[p->currentColorTable].colors[p->names[name]];
}

QVariant S52::GetAttribute(const QString &name, const S57::Object *obj) {
  Private::Presentation* p = Private::Presentation::instance();
  Q_ASSERT(p->names.contains(name));

  if (!obj->attributes().contains(p->names[name])) return QVariant();

  return obj->attributes()[p->names[name]].value();
}


