/* -*- coding: utf-8-unix -*-
 *
 * File: src/s52presentation.cpp
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
#include "s52presentation_p.h"
#include "logging.h"
#include <QDir>
#include <QStandardPaths>
#include "s52names.h"
#include <functional>
#include "units.h"

void S52::Lookup::run_bytecode(const S57::Object *obj, Accumulator accumulate) const {

  int stackPos = 0;
  int immedPos = 0;
  int refPos = 0;

  S52::Lookup::ValueStack stack(20);

  for (auto code: m_code) {

    switch (code) {

    case Code::Immed:
      stack[stackPos++] = m_immed[immedPos++];
      break;

    case Code::Fun: {
      auto fun = S52::FindFunction(m_references[refPos++]);
      // qCDebug(CS52) << "function" << fun->name();
      stackPos = 0;
      accumulate(fun, stack, obj);
      break;
    }

    case Code::Var:
      stack[stackPos++] = obj->attributeValue(m_references[refPos++]);
      break;

    case Code::DefVar: {
      QVariant v = obj->attributeValue(m_references[refPos++]);
      QVariant d = m_immed[immedPos++];
      if (v.isValid()) {
        stack[stackPos++] = v;
      } else {
        stack[stackPos++] = d;
      }
      break;
    }
    default:
      Q_ASSERT(false);
    }
  }
}

S57::PaintDataMap S52::Lookup::execute(const S57::Object *obj) const {

  S57::PaintDataMap ps;

  auto accum = [&ps] (S52::Function* fun, const ValueStack& values, const S57::Object* thing) {
    ps += fun->execute(values, thing);
  };

  run_bytecode(obj, accum);

  return ps;
}

S57::PaintDataMap S52::Lookup::modifiers(const S57::Object *obj) const {

  S57::PaintDataMap ps;

  auto accum = [&ps] (S52::Function* fun, const ValueStack& values, const S57::Object* thing) {
    ps += fun->modifiers(values, thing);
  };

  run_bytecode(obj, accum);

  return ps;
}

void S52::Lookup::paintIcon(PickIconData& icon, const S57::Object* obj) const {

  auto accum = [&icon] (S52::Function* fun, const ValueStack& values, const S57::Object* thing) {
    fun->paintIcon(icon, values, thing);
  };

  run_bytecode(obj, accum);
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
  qCDebug(CS52) << "No match for" << S52::GetClassInfo(code) << obj->name();
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


void S52::InitPresentation() {
  InitNames();
  Private::Presentation* p = Private::Presentation::instance();
  p->init();
}

QString S52::GetSymbolInfo(quint32 index, S52::SymbolType t) {
  return GetSymbolInfo(SymbolKey(index, t));
}

QString S52::GetSymbolInfo(const SymbolKey& key) {
  const Private::Presentation* p = Private::Presentation::instance();
  if (!p->symbols.contains(key)) return QString();
  return p->symbols[key].code + ": " + p->symbols[key].description;
}

QString S52::GetAttributeInfo(quint32 index, const S57::Object* obj) {
  auto code = GetAttributeName(index);
  if (code.isEmpty()) return QString();

  auto attr = GetAttributeDescription(index);
  QString info;
  info += code + ": " + attr + ": ";
  QVariant v = obj->attributeValue(index);
  switch (GetAttributeType(index)) {
  case S57::Attribute::Type::Real:
    info += QString::number(v.toDouble());
    break;
  case S57::Attribute::Type::String:
    info += v.toString();
    break;
  case S57::Attribute::Type::Integer: {
    auto descr = GetAttributeEnumDescription(index, v.toInt());
    if (!descr.isEmpty()) {
      info += descr;
    } else {
      info += QString::number(v.toInt());
    }
    break;
  }
  case S57::Attribute::Type::IntegerList:
  {
    auto items = v.toList();
    for (auto a: items) {
      auto descr = GetAttributeEnumDescription(index, a.toInt());
      if (!descr.isEmpty()) {
        info += descr + ", ";
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

void S52::ParseInstruction(Lookup* lup, bool* ok) {
  Private::Presentation* p = Private::Presentation::instance();
  int err = p->parseInstruction(lup);
  if (ok != nullptr) *ok = err == 0;
}



static void attributeDesc(QStringList& parts, quint32 aid, const S57::Object* obj, const int skipVal = -1) {

  static const QSet<quint32> depths {
    S52::FindCIndex("VALDCO"),
    S52::FindCIndex("DRVAL1"),
    S52::FindCIndex("VALSOU")
  };
  static const QSet<quint32> lengths {S52::FindCIndex("HEIGHT")};

  const QVariant v = obj->attributeValue(aid);
  if (!v.isValid()) return;

  switch (S52::GetAttributeType(aid)) {
  case S57::Attribute::Type::Real: {
    const auto v = obj->attributeValue(aid).toDouble();
    if (depths.contains(aid)) {
      parts << Units::Manager::instance()->depth()->displaySI(v, 2);
    } else if (lengths.contains(aid)) {
      parts << Units::Manager::instance()->shortDistance()->displaySI(v, 1);
    } else {
      Q_ASSERT(false);
    }
    break;
  }
  case S57::Attribute::Type::Integer: {
    const auto eid = obj->attributeValue(aid).toInt();
    if (skipVal < 0 || eid != skipVal) {
      parts << S52::GetAttributeEnumDescription(aid, eid);
    }
    break;
  }
  case S57::Attribute::Type::IntegerList: {
    auto items = obj->attributeValue(aid).toList();
    QStringList vs;
    for (auto a: items) {
      const auto eid = a.toInt();
      if (skipVal < 0 || eid != skipVal) {
        vs << S52::GetAttributeEnumDescription(aid, eid);
      }
    }
    if (!vs.isEmpty()) parts << vs.join(", ");
    break;
  }
  case S57::Attribute::Type::String: {
    parts << obj->attributeValue(aid).toString();
    break;
  }
  default:
    Q_ASSERT(false);
  }
}

static QString desc_rectrc(const S57::Object* obj) {
  static auto drval1 = S52::FindCIndex("DRVAL1");
  const auto base = S52::GetClassDescription(obj->classCode());
  if (obj->attributeValue(drval1).isValid()) {
    const auto v = obj->attributeValue(drval1).toDouble();
    return QString("%1 (%2)")
        .arg(base)
        .arg(Units::Manager::instance()->depth()->displaySI(v, 2));
  }
  return base;
}

static QString desc_depcnt(const S57::Object* obj) {
  static auto valdco = S52::FindCIndex("VALDCO");
  static auto base = S52::GetClassDescription(obj->classCode());
  const auto v = obj->attributeValue(valdco).toDouble();
  return QString("%1 (%2)")
      .arg(base)
      .arg(Units::Manager::instance()->depth()->displaySI(v, 2));
}

static QString desc_bcnspp(const S57::Object* obj) {
  static const QVector<QPair<quint32, int>> attrs = {
    {S52::FindCIndex("BCNSHP"), -1},
    {S52::FindCIndex("CATSPM"), 52},
    {S52::FindCIndex("COLPAT"), -1},
    {S52::FindCIndex("COLOUR"), -1},
    {S52::FindCIndex("HEIGHT"), -1},
    {S52::FindCIndex("STATUS"), 1}, // permanent
  };

  QStringList parts;
  for (const auto& a: attrs) {
    attributeDesc(parts, a.first, obj, a.second);
  }
  return parts.join("; ");
}

static QString desc_bcnlat(const S57::Object* obj) {
  static const QVector<QPair<quint32, int>> attrs = {
    {S52::FindCIndex("CATLAM"), -1},
    {S52::FindCIndex("BCNSHP"), -1},
    {S52::FindCIndex("HEIGHT"), -1},
    {S52::FindCIndex("STATUS"), 1},
  };

  QStringList parts;
  for (auto a: attrs) {
    attributeDesc(parts, a.first, obj, a.second);
  }
  return parts.join("; ");
}

static QString desc_bcncar(const S57::Object* obj) {
  static const QVector<QPair<quint32, int>> attrs = {
    {S52::FindCIndex("CATCAM"), -1},
    {S52::FindCIndex("BCNSHP"), -1},
    {S52::FindCIndex("HEIGHT"), -1},
    {S52::FindCIndex("STATUS"), 1},
  };

  QStringList parts;
  for (auto a: attrs) {
    attributeDesc(parts, a.first, obj, a.second);
  }
  return parts.join("; ");
}

static QString desc_boyspp(const S57::Object* obj) {
  static const QVector<QPair<quint32, int>> attrs = {
    {S52::FindCIndex("BOYSHP"), -1},
    {S52::FindCIndex("CATSPM"), 52},
    {S52::FindCIndex("COLPAT"), -1},
    {S52::FindCIndex("COLOUR"), -1},
    {S52::FindCIndex("STATUS"), 1},
  };

  QStringList parts;
  for (const auto& a: attrs) {
    attributeDesc(parts, a.first, obj, a.second);
  }
  return parts.join("; ");
}


static QString desc_boylat(const S57::Object* obj) {
  static const QVector<QPair<quint32, int>> attrs = {
    {S52::FindCIndex("CATLAM"), -1},
    {S52::FindCIndex("BOYSHP"), -1},
    {S52::FindCIndex("STATUS"), 1},
  };

  QStringList parts;
  for (const auto& a: attrs) {
    attributeDesc(parts, a.first, obj, a.second);
  }
  return parts.join("; ");
}

static QString desc_boycar(const S57::Object* obj) {
  static const QVector<QPair<quint32, int>> attrs = {
    {S52::FindCIndex("CATCAM"), -1},
    {S52::FindCIndex("BOYSHP"), -1},
    {S52::FindCIndex("STATUS"), 1},
  };

  QStringList parts;
  for (const auto& a: attrs) {
    attributeDesc(parts, a.first, obj, a.second);
  }
  return parts.join("; ");
}

static QString desc_mipare(const S57::Object* obj) {
  static const QVector<quint32> attrs = {
    S52::FindCIndex("CATMPA"),
    S52::FindCIndex("STATUS"),
  };

  QStringList parts;
  for (auto aid: attrs) {
    attributeDesc(parts, aid, obj);
  }
  return parts.join("; ");
}

static QString desc_magvar(const S57::Object* obj) {
  static auto valmag = S52::FindCIndex("VALMAG");
  static auto ryrmgv = S52::FindCIndex("RYRMGV");
  static auto valacm = S52::FindCIndex("VALACM");

  const auto year = obj->attributeValue(ryrmgv).toString();
  const auto val = Angle::fromDegrees(obj->attributeValue(valmag).toDouble());
  const auto ann = Angle::fromDegrees(obj->attributeValue(valacm).toDouble() / 60.);

  // value year (annual change)
  return QString("%1: %2 %3 (%4)")
      .arg(S52::GetClassDescription(obj->classCode()))
      .arg(val.printAsLongitude())
      .arg(year)
      .arg(ann.printAsLongitude());
}

static QString desc_resare(const S57::Object* obj) {
  static const QVector<quint32> attrs = {
    S52::FindCIndex("CATREA"),
    S52::FindCIndex("RESTRN"),
    S52::FindCIndex("STATUS"),
  };

  QStringList parts;
  for (auto aid: attrs) {
    attributeDesc(parts, aid, obj);
  }
  return parts.join("; ");
}

static QString desc_achare(const S57::Object* obj) {
  static const QVector<QPair<quint32, int>> attrs = {
    {S52::FindCIndex("CATACH"), -1},
    {S52::FindCIndex("RESTRN"), -1},
    {S52::FindCIndex("STATUS"), 1},
  };

  QStringList parts;
  for (const auto& a: attrs) {
    attributeDesc(parts, a.first, obj, a.second);
  }
  return parts.join("; ");
}

static QString desc_newobj(const S57::Object* obj) {
  static const QVector<quint32> attrs = {
    S52::FindCIndex("CLSNAM"),
    S52::FindCIndex("RESTRN"),
    S52::FindCIndex("STATUS"),
  };

  QStringList parts;
  for (auto aid: attrs) {
    attributeDesc(parts, aid, obj);
  }
  return parts.join("; ");
}

static QString desc_lndmrk(const S57::Object* obj) {
  static const QVector<QPair<quint32, int>> attrs = {
    {S52::FindCIndex("CATLMK"), -1},
    {S52::FindCIndex("FUNCTN"), 1},
    {S52::FindCIndex("COLPAT"), -1},
    {S52::FindCIndex("COLOUR"), -1},
    {S52::FindCIndex("HEIGHT"), -1},
    {S52::FindCIndex("STATUS"), 1},
  };

  QStringList parts;
  for (const auto& a: attrs) {
    attributeDesc(parts, a.first, obj, a.second);
  }
  return parts.join("; ");
}

static QString desc_sistaw(const S57::Object* obj) {
  static const QVector<quint32> attrs = {
    S52::FindCIndex("CATSIW"),
    S52::FindCIndex("COMCHA"),
    S52::FindCIndex("STATUS"),
  };

  QStringList parts;
  for (auto aid: attrs) {
    attributeDesc(parts, aid, obj);
  }
  return S52::GetClassDescription(obj->classCode()) + ": " + parts.join("; ");
}

static QString desc_wrecks(const S57::Object* obj) {
  static const QVector<quint32> attrs = {
    S52::FindCIndex("CATWRK"),
    S52::FindCIndex("WATLEV"),
    S52::FindCIndex("VALSOU"),
    S52::FindCIndex("EXPSOU"),
    S52::FindCIndex("QUASOU"),
    S52::FindCIndex("STATUS"),
  };

  QStringList parts;
  for (auto aid: attrs) {
    attributeDesc(parts, aid, obj);
  }
  return parts.join("; ");
}

static QString desc_uwtroc(const S57::Object* obj) {
  static const QVector<quint32> attrs = {
    S52::FindCIndex("NATSUR"),
    S52::FindCIndex("WATLEV"),
    S52::FindCIndex("VALSOU"),
    S52::FindCIndex("EXPSOU"),
    S52::FindCIndex("QUASOU"),
    S52::FindCIndex("STATUS"),
  };

  QStringList parts;
  for (auto aid: attrs) {
    attributeDesc(parts, aid, obj);
  }
  return parts.join("; ");
}

static QString desc_obstrn(const S57::Object* obj) {
  static const QVector<quint32> attrs = {
    S52::FindCIndex("CATOBS"),
    S52::FindCIndex("NATSUR"),
    S52::FindCIndex("WATLEV"),
    S52::FindCIndex("VALSOU"),
    S52::FindCIndex("EXPSOU"),
    S52::FindCIndex("QUASOU"),
    S52::FindCIndex("STATUS"),
  };

  QStringList parts;
  for (auto aid: attrs) {
    attributeDesc(parts, aid, obj);
  }
  return parts.join("; ");
}

static QString desc_morfac(const S57::Object* obj) {
  static const QVector<QPair<quint32, int>> attrs = {
    {S52::FindCIndex("CATMOR"), -1},
    {S52::FindCIndex("WATLEV"), -1},
    {S52::FindCIndex("COLOUR"), -1},
    {S52::FindCIndex("HEIGHT"), -1},
    {S52::FindCIndex("STATUS"), 1},
  };

  QStringList parts;
  for (const auto& a: attrs) {
    attributeDesc(parts, a.first, obj, a.second);
  }
  return parts.join("; ");
}

static QString desc_bridge(const S57::Object* obj) {
  static const QVector<quint32> attrs = {
    S52::FindCIndex("CATBRG"),
    S52::FindCIndex("VERCLR"),
  };

  QStringList parts;
  for (auto aid: attrs) {
    attributeDesc(parts, aid, obj);
  }
  return parts.join("; ");
}

static QString desc_depare(const S57::Object* obj) {
  static auto drval1 = S52::FindCIndex("DRVAL1");
  static auto drval2 = S52::FindCIndex("DRVAL2");
  static auto base = S52::GetClassDescription(obj->classCode());

  const auto d1 = obj->attributeValue(drval1);
  const auto d2 = obj->attributeValue(drval2);
  if (!d1.isValid() || !d2.isValid()) {
    return base;
  }

  const auto v1 = d1.toDouble();

  const auto v2 = d2.toDouble();
  const auto s2 = Units::Manager::instance()->depth()->displaySI(v2, 2);

  if (v1 == 0) {
    return QString("%1 (0 - %2)").arg(base).arg(s2);
  }

  const auto s1 = Units::Manager::instance()->depth()->displaySI(v1, 2, false);
  return QString("%1 (%2 - %3)").arg(base).arg(s1).arg(s2);
}


using Descriptor = std::function<QString (const S57::Object*)>;

QString S52::ObjectDescription(const S57::Object* obj) {
  static const QMap<quint32, Descriptor> descriptions = {
    {S52::FindCIndex("RECTRC"), desc_rectrc},
    {S52::FindCIndex("TWRTPT"), desc_rectrc}, // reuses rectrc
    {S52::FindCIndex("DEPCNT"), desc_depcnt},
    {S52::FindCIndex("BCNSPP"), desc_bcnspp},
    {S52::FindCIndex("BCNLAT"), desc_bcnlat},
    {S52::FindCIndex("BCNCAR"), desc_bcncar},
    {S52::FindCIndex("BOYSPP"), desc_boyspp},
    {S52::FindCIndex("BOYLAT"), desc_boylat},
    {S52::FindCIndex("BOYCAR"), desc_boycar},
    {S52::FindCIndex("MIPARE"), desc_mipare},
    {S52::FindCIndex("MAGVAR"), desc_magvar},
    {S52::FindCIndex("RESARE"), desc_resare},
    {S52::FindCIndex("ACHARE"), desc_achare},
    {S52::FindCIndex("NEWOBJ"), desc_newobj},
    {S52::FindCIndex("LNDMRK"), desc_lndmrk},
    {S52::FindCIndex("SISTAW"), desc_sistaw},
    {S52::FindCIndex("WRECKS"), desc_wrecks},
    {S52::FindCIndex("UWTROC"), desc_uwtroc},
    {S52::FindCIndex("OBSTRN"), desc_obstrn},
    {S52::FindCIndex("MORFAC"), desc_morfac},
    {S52::FindCIndex("BRIDGE"), desc_bridge},
    {S52::FindCIndex("DEPARE"), desc_depare},
  };

  const auto code = obj->classCode();

  if (descriptions.contains(code)) return descriptions[code](obj);

  return S52::GetClassDescription(code);
}
