#include "s52presentation_p.h"
#include <QStandardPaths>
#include <QDir>
#include <QTextStream>
#include <QDebug>
#include "conf_marinerparams.h"
#include "s52instr_parser.h"
#define YYLTYPE Private::LocationType
#define YYSTYPE Private::Instr_ValueType
#include "s52instr_scanner.h"

void s52hpgl_error(Private::LocationType* loc,
                   Private::Presentation* p,
                   yyscan_t scanner, const char*  msg) {
  s52instr_error(loc, p, 0, scanner, msg);
}

void s52instr_error(Private::LocationType* loc,
                    Private::Presentation*,
                    S52::Lookup*,
                    yyscan_t,
                    const char *msg) {
  QString err("Error at position %1-%2: %3");
  qWarning() << err.arg(loc->pos).arg(loc->prev_pos).arg(msg);
}

Private::Presentation::Presentation()
  : QObject()
  , m_nextSymbolIndex(0)
  , functions(nullptr)
  , settings(Settings::instance()) {

  readAttributes();
  readObjectClasses();
  readChartSymbols();

  connect(settings, &Settings::colorTable, this, &Presentation::setColorTable);
  connect(settings, &Settings::plainBoundaries, this, &Presentation::setPlainBoundaries);
  connect(settings, &Settings::simplifiedSymbols, this, &Presentation::setSimplifiedSymbols);

  setColorTable(Conf::MarinerParams::colorTable());
  setPlainBoundaries(Conf::MarinerParams::plainBoundaries());
  setSimplifiedSymbols(Conf::MarinerParams::simplifiedSymbols());
}


void Private::Presentation::setColorTable(Settings::ColorTable t) {
  const QMap<Settings::ColorTable, QString> tables{
    {Settings::DayBright, "DAY_BRIGHT"},
    {Settings::DayBlackBack, "DAY_BLACKBACK"},
    {Settings::DayWhiteBack, "DAY_WHITEBACK"},
    {Settings::Dusk, "DUSK"},
    {Settings::Night, "NIGHT"},
  };
  currentColorTable = names[tables[t]];
}

void Private::Presentation::setPlainBoundaries(bool v) {
  plainBoundaries = v;
}

void Private::Presentation::setSimplifiedSymbols(bool v) {
  simplifiedSymbols = v;
}

Private::Presentation* Private::Presentation::instance() {
  static Presentation* p = new Presentation();
  return p;
}

int Private::Presentation::parseInstruction(S52::Lookup* lup) {
  if (lup->byteCodeReady()) return 0;

  const QString src = lup->source();
  if (src.isEmpty()) return 0;

  yyscan_t scanner;
  s52instr_lex_init(&scanner);
  s52instr__scan_string(src.toUtf8().constData(), scanner);
  int err = s52instr_parse(this, lup, scanner);
  s52instr_lex_destroy(scanner);

  return err;
}

S52::Lookup::Type Private::Presentation::typeFilter(const S57::Object *obj) const {
  const S57::Geometry::Type t = obj->geometry()->type();
  if (t == S57::Geometry::Type::Line) return S52::Lookup::Type::Lines;
  if (t == S57::Geometry::Type::Area) {
    return plainBoundaries ? S52::Lookup::Type::PlainBoundaries :
                             S52::Lookup::Type::SymbolizedBoundaries;
  }
  return simplifiedSymbols ? S52::Lookup::Type::Simplified :
                             S52::Lookup::Type::PaperChart;
}


void Private::Presentation::readObjectClasses() {
  QFile file(S52::FindPath("s57objectclasses.csv"));
  file.open(QFile::ReadOnly);
  QTextStream s(&file);

  while (!s.atEnd()) {
    const QStringList parts = s.readLine().split(",");
    bool ok;
    const quint32 classCode = parts[0].toUInt(&ok);
    if (!ok) continue;
    QString description = parts[1];
    int i = 1;
    while (!description.endsWith("\"")) {
      i += 1;
      description += ",";
      description += parts[i];
    }
    description.replace("\"", "");
    const QString className = parts[i + 1];

    if (names.contains(className)) {
      qWarning() << className << "already parsed";
      continue;
    }

    names[className] = classCode;
    classes[classCode] = ClassDescription(className, description);

  }

  // Object class for unknowns
  const quint32 unknownCode = 666666;
  const QString unknownClass("######");
  names[unknownClass] = unknownCode;
  classes[unknownCode] = ClassDescription(unknownClass, "Unknown object class");

  file.close();
}


void Private::Presentation::readAttributes() {
  const QMap<QString, S57::Attribute::Type> typeLookup {
    {"A", S57::Attribute::Type::String},
    {"I", S57::Attribute::Type::Integer},
    {"S", S57::Attribute::Type::String},
    {"E", S57::Attribute::Type::Integer},
    {"L", S57::Attribute::Type::IntegerList},
    {"F", S57::Attribute::Type::Real},
  };

  QFile afile(S52::FindPath("s57attributes.csv"));
  afile.open(QFile::ReadOnly);
  QTextStream s1(&afile);

  while (!s1.atEnd()) {
    const QStringList parts = s1.readLine().split(",");
    if (parts.length() != 5) continue;
    bool ok;
    const quint32 id = parts[0].toUInt(&ok);
    if (!ok) continue;
    const QString description = parts[1];
    const QString attributeName = parts[2];
    if (names.contains(attributeName)) {
      qWarning() << attributeName << "already parsed";
      continue;
    }
    const S57::Attribute::Type t = typeLookup[parts[3]];

    names[attributeName] = id;
    attributes[id] = AttributeDescription(attributeName, t, description);
  }
  afile.close();

  QFile dfile(S52::FindPath("attdecode.csv"));
  dfile.open(QFile::ReadOnly);
  QTextStream s2(&dfile);

  while (!s2.atEnd()) {
    const QStringList parts = s2.readLine().split(",");
    if (parts.length() != 2) continue;
    const QString attributeName = parts[0];
    if (attributeName.startsWith("\"")) continue; // skip description line
    if (!names.contains(attributeName)) {
      qWarning() << attributeName << "not found";
      continue;
    }
    DescriptionMap* target = &attributes[names[attributeName]].enumDescriptions;
    QStringList tokens = parts[1].split(";");
    for (int i = 0; i < tokens.length() / 2; i++) {
      (*target)[tokens[2 * i].toUInt()] = tokens[2 * i + 1];
    }
  }
  dfile.close();
}

void Private::Presentation::readChartSymbols() {
  QFile file(S52::FindPath("chartsymbols.xml"));
  file.open(QFile::ReadOnly);
  QXmlStreamReader reader(&file);

  reader.readNextStartElement();
  Q_ASSERT(reader.name() == "chartsymbols");

  while (reader.readNextStartElement()) {
    if (reader.name() == "color-tables") {
      readColorTables(reader);
    } else if (reader.name() == "lookups") {
      readLookups(reader);
    } else if (reader.name() == "line-styles") {
      readSymbolNames(reader);
    } else if (reader.name() == "patterns") {
      readSymbolNames(reader);
    } else if (reader.name() == "symbols") {
      readSymbolNames(reader);
    }
  }
  file.close();
}

void Private::Presentation::readColorTables(QXmlStreamReader& reader) {
  while (reader.readNextStartElement()) {
    Q_ASSERT(reader.name() == "color-table");
    const QString tableName = reader.attributes().value("name").toString();
    if (names.contains(tableName)) {
      qWarning() << tableName << "already parsed";
      reader.skipCurrentElement();
      continue;
    }
    names[tableName] = colorTables.size();
    reader.readNextStartElement();
    Q_ASSERT(reader.name() == "graphics-file");

    ColorTable colors(reader.attributes().value("name").toString());
    if (!colorTables.isEmpty()) {
      colors.colors.resize(colorTables.first().colors.size());
    }
    reader.skipCurrentElement();
    while (reader.readNextStartElement()) {
      Q_ASSERT(reader.name() == "color");

      const QString colorName = reader.attributes().value("name").toString();
      const QColor color(reader.attributes().value("r").toInt(),
                         reader.attributes().value("g").toInt(),
                         reader.attributes().value("b").toInt());
      if (names.contains(colorName)) {
        colors.colors[names[colorName]] = color;
      } else {
        names[colorName] = colors.colors.size();
        colors.colors.append(color);
      }
      reader.skipCurrentElement();
    }
    colorTables.append(colors);
  }
}

void Private::Presentation::readLookups(QXmlStreamReader& reader) {
  const QMap<QString, S52::Lookup::Type> typeLookup {
    {"Lines", S52::Lookup::Type::Lines},
    {"Paper", S52::Lookup::Type::PaperChart},
    {"Plain", S52::Lookup::Type::PlainBoundaries},
    {"Simplified", S52::Lookup::Type::Simplified},
    {"Symbolized", S52::Lookup::Type::SymbolizedBoundaries},
  };
  const QMap<QString, int> prioLookup {
    {"Area 1", 2},
    {"Area 2", 3},
    {"Area Symbol", 6},
    {"Group 1", 1},
    {"Hazards", 8},
    {"Line Symbol", 5},
    {"Mariners", 9},
    {"No data", 0},
    {"Point Symbol", 4},
    {"Routing", 7},
  };
  const QMap<QString, S52::Lookup::Category> catLookup {
    {"Displaybase", S52::Lookup::Category::Base},
    {"Mariners", S52::Lookup::Category::Mariners},
    {"Other", S52::Lookup::Category::Other},
    {"Standard", S52::Lookup::Category::Standard},
  };

  LookupTable unsortedLookups;

  while (reader.readNextStartElement()) {
    Q_ASSERT(reader.name() == "lookup");

    const int id = reader.attributes().value("RCID").toInt();
    const QString className = reader.attributes().value("name").toString();

    if (!names.contains(className)) {
      qWarning() << "Unknown class name" << className << ", skipping lookup" << id;
      reader.skipCurrentElement();
      continue;
    }

    const int code = names[className];
    S52::Lookup::Type name;
    int prio;
    S52::Lookup::Category cat;
    QString comment;
    QString instr;
    S52::Lookup::AttributeMap attrs;

    const QStringList ignored {"type", "radar-prio"};

    bool ok = true;
    while (reader.readNextStartElement()) {
      if (reader.name() == "disp-prio") {
        prio = prioLookup[reader.readElementText()];
      } else if (reader.name() == "table-name") {
        name = typeLookup[reader.readElementText()];
      } else if (reader.name() == "display-cat") {
        cat = catLookup[reader.readElementText()];
      } else if (reader.name() == "comment") {
        comment = reader.readElementText();
      } else if (reader.name() == "instruction") {
        instr = reader.readElementText();
      } else if (reader.name() == "attrib-code") {
        QString token = reader.readElementText().trimmed();
        QString key = token.left(6);
        if (!names.contains(key)) {
          qWarning() << "Unknown attribute" << key << ", skipping lookup" << id;
          ok = false;
          break;
        }
        QString value = token.mid(6).simplified();
        quint32 id = names[key];
        if (value.isEmpty()) {
          attrs[id] = S57::Attribute(S57::Attribute::Type::Any);
        } else if (value == "?") {
          attrs[id] = S57::Attribute(S57::Attribute::Type::None);
        } else if (attributes[id].type == S57::Attribute::Type::Integer) {
          attrs[id] = S57::Attribute(value.toInt());
        } else if (attributes[id].type == S57::Attribute::Type::IntegerList) {
          QVariantList values;
          QStringList parts = value.split(",");
          for (auto p: parts) values << QVariant::fromValue(p.toInt());
          attrs[id] = S57::Attribute(values);
        } else if (attributes[id].type == S57::Attribute::Type::Real) {
          attrs[id] = S57::Attribute(value.toDouble());
        } else if (attributes[id].type == S57::Attribute::Type::String) {
          attrs[id] = S57::Attribute(value);
        }
      } else if (ignored.contains(reader.name())) {
        reader.skipCurrentElement();
      } else {
        qWarning() << "Don't know how to handle" << reader.name();
        reader.skipCurrentElement();
      }
    }
    if (!ok) {
      reader.skipCurrentElement();
      continue;
    }
    auto lookup = new S52::Lookup(name, id, code, prio, cat, attrs, comment, instr);

    unsortedLookups[name][code].append(lookup);
  }

  // sort lookups
  LUPTableIterator tables(unsortedLookups.cbegin());
  while (tables != unsortedLookups.cend()) {
    const S52::Lookup::Type& table = tables.key();
    const LookupHash& classes = tables.value();
    LUPHashIterator cl(classes.cbegin());
    while (cl != classes.cend()) {
      const quint32& feature = cl.key();
      LookupVector lups = cl.value();
      std::sort(lups.begin(), lups.end(), [] (const S52::Lookup* s1, const S52::Lookup* s2) {
        if (s1->attributes().size() != s2->attributes().size()) {
          return s1->attributes().size() > s2->attributes().size();
        }
        return s1->rcid() < s2->rcid();
      });
      lookupTable[table][feature] = lups;
      ++cl;
    }
    ++tables;
  }
}

S52::ColorRef Private::Presentation::parseColorRef(QXmlStreamReader& reader) {
  S52::ColorRef ref;
  const QString src = reader.readElementText();
  int index = 0;
  while (index < src.length()) {
    const char key = src.mid(index, 1).front().toLatin1();
    index += 1;
    const QString cref = src.mid(index, 5);
    if (names.contains(cref)) {
      ref[key] = names[cref];
    }
    index += 5;
  }
  return ref;
}

void Private::Presentation::readSymbolNames(QXmlStreamReader& reader) {
  const QMap<QString, S52::SymbolType> typeLookup {
    {"line-style", S52::SymbolType::LineStyle},
    {"pattern", S52::SymbolType::Pattern},
    {"symbol", S52::SymbolType::Single},
  };

  while (reader.readNextStartElement()) {
    Q_ASSERT(reader.name() == "line-style" ||
             reader.name() == "pattern" ||
             reader.name() == "symbol");

    S52::SymbolType t = typeLookup[reader.name().toString()];

    QString name;
    QString descr;
    while (reader.readNextStartElement()) {
      if (reader.name() == "name") {
        name = reader.readElementText();
      } else if (reader.name() == "description") {
        descr = reader.readElementText();
      } else {
        reader.skipCurrentElement();
      }
    }
    if (!names.contains(name)) {
      names[name] = m_nextSymbolIndex++;
    }
    quint32 index = names[name];
    const SymbolKey key(index, t);
    if (!symbols.contains(key)) {
      symbols.insert(key, SymbolDescription(name, descr));
    }
  }
}


void Private::Presentation::init() {

  functions = new S52::Functions();

  for (LUPTableIterator tables(lookupTable.cbegin()); tables != lookupTable.cend(); ++tables) {
    const LookupHash& classes = tables.value();
    LUPHashIterator cl(classes.cbegin());
    for (LUPHashIterator cl(classes.cbegin()); cl != classes.cend(); ++cl) {
      LookupVector lups = cl.value();
      for (S52::Lookup* lup: lups) {
        int err = parseInstruction(lup);
        if (err != 0) {
          qWarning() << "Error parsing" << lup->source();
        }
      }
    }
  }
}
