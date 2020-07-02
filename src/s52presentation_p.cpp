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
                   S52::LineStyle*,
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


static QString findFile(const QString& s) {
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

void Private::Presentation::readObjectClasses() {
  QFile file(findFile("s57objectclasses.csv"));
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
    classDescriptions[classCode] = description;

  }

  // Object class for unknowns
  const quint32 unknownCode = 666666;
  names["######"] = unknownCode;
  classDescriptions[unknownCode] = "Unknown object class";

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

  QFile afile(findFile("s57attributes.csv"));
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
    attributes[id] = AttributeDescription(t, description);
  }
  afile.close();

  QFile dfile(findFile("attdecode.csv"));
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
  QFile file(findFile("chartsymbols.xml"));
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
      readLineStyles(reader);
    } else if (reader.name() == "patterns") {
      readPatterns(reader);
    } else if (reader.name() == "symbols") {
      readSymbols(reader);
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
        QString value = token.mid(6);
        quint32 id = names[key];
        if (value.isEmpty()) {
          attrs[id] = S57::Attribute(S57::Attribute::Type::Any);
        } else if (value == "?") {
          attrs[id] = S57::Attribute(S57::Attribute::Type::None);
        } else if (attributes[id].type == S57::Attribute::Type::Integer) {
          attrs[id] = S57::Attribute(value.toInt());
        } else if (attributes[id].type == S57::Attribute::Type::IntegerList) {
          QVector<int> values;
          QStringList parts = value.split(",");
          for (auto p: parts) values << p.toInt();
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
    lookupTable[name].insert(code, lookups.size());
    lookups.append(lookup);
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

static S52::ModelData parseModelData(QXmlStreamReader& reader, QPoint* ref, QString* hpgl) {
  int w = reader.attributes().value("width").toInt();
  int h = reader.attributes().value("height").toInt();
  int minD;
  int maxD;
  QPoint p;
  QPoint o;

  while (reader.readNextStartElement()) {
    if (reader.name() == "distance") {
      minD = reader.attributes().value("min").toInt();
      maxD = reader.attributes().value("min").toInt();
      reader.skipCurrentElement();
    } else if (reader.name() == "pivot") {
      p = QPoint(reader.attributes().value("x").toInt(),
                 reader.attributes().value("y").toInt());
      reader.skipCurrentElement();
    } else if (reader.name() == "origin") {
      o = QPoint(reader.attributes().value("x").toInt(),
                 reader.attributes().value("y").toInt());
      reader.skipCurrentElement();
    } else if (reader.name() == "graphics-location") {
      if (ref) {
        ref->setX(reader.attributes().value("x").toInt());
        ref->setY(reader.attributes().value("y").toInt());
      }
      reader.skipCurrentElement();
    } else if (reader.name() == "HPGL") {
      if (hpgl) {
        *hpgl = reader.readElementText();
      } else {
        reader.skipCurrentElement();
      }
    } else {
      qWarning() << "Don't know how to handle" << reader.name();
      reader.skipCurrentElement();
    }
  }

  return S52::ModelData(w, h, minD, maxD, p, o);
}

void Private::Presentation::readLineStyles(QXmlStreamReader& reader) {
  while (reader.readNextStartElement()) {
    Q_ASSERT(reader.name() == "line-style");

    QString styleName;
    int id = reader.attributes().value("RCID").toInt();
    S52::ModelData d;
    QString description;
    S52::ColorRef ref;
    QString instruction;

    bool ok = true;
    while (reader.readNextStartElement()) {
      if (reader.name() == "name") {
        styleName = reader.readElementText();
        if (names.contains(styleName) && lineStyles.contains(names[styleName])) {
          qWarning() << styleName << "already parsed, skipping";
          ok = false;
          break;
        }
      } else if (reader.name() == "vector") {
        d = parseModelData(reader, nullptr, nullptr);
      } else if (reader.name() == "description") {
        description = reader.readElementText();
      } else if (reader.name() == "HPGL") {
        instruction = reader.readElementText();
      } else if (reader.name() == "color-ref") {
        ref = parseColorRef(reader);
      } else {
        qWarning() << "Don't know how to handle" << reader.name();
        reader.skipCurrentElement();
      }
    }
    if (!ok) {
      reader.skipCurrentElement();
      continue;
    }
    if (!names.contains(styleName)) {
      names[styleName] = m_nextSymbolIndex++;
    }
    lineStyles[names[styleName]] = S52::LineStyle(id, d, description, ref, instruction);
  }
}

void Private::Presentation::readPatterns(QXmlStreamReader& reader) {
  while (reader.readNextStartElement()) {
    Q_ASSERT(reader.name() == "pattern");

    QString patternName;
    int id = reader.attributes().value("RCID").toInt();
    S52::ModelData dv;
    S52::ModelData dr;
    QString description;
    S52::ColorRef ref;
    QString instruction;
    QPoint gref;
    bool staggered;
    bool raster;
    bool variable;

    while (reader.readNextStartElement()) {
      if (reader.name() == "name") {
        patternName = reader.readElementText();
      } else if (reader.name() == "definition") {
        raster = reader.readElementText() == "R";
      } else if (reader.name() == "filltype") {
        staggered = reader.readElementText() != "L";
      } else if (reader.name() == "spacing") {
        variable = reader.readElementText() != "C";
      } else if (reader.name() == "vector") {
        dv = parseModelData(reader, nullptr, nullptr);
      } else if (reader.name() == "bitmap") {
        dr = parseModelData(reader, &gref, nullptr);
      } else if (reader.name() == "description") {
        description = reader.readElementText();
      } else if (reader.name() == "HPGL") {
        instruction = reader.readElementText();
      } else if (reader.name() == "color-ref") {
        ref = parseColorRef(reader);
      } else {
        qWarning() << "Don't know how to handle" << reader.name();
        reader.skipCurrentElement();
      }
    }
    if (names.contains(patternName) && patterns.contains(names[patternName])) {
      if (!patterns[names[patternName]].isRaster() || raster) {
        qWarning() << patternName << "already parsed, skipping";
        continue;
      }
    }

    if (!names.contains(patternName)) {
      names[patternName] = m_nextSymbolIndex++;
    }
    if (raster) {
      QString src = QString("%1 %2").arg(gref.x()).arg(gref.y());
      patterns[names[patternName]] = S52::Pattern(id, dr, description, ref, src, raster, staggered, variable);
    } else {
      patterns[names[patternName]] = S52::Pattern(id, dv, description, ref, instruction, raster, staggered, variable);
    }
  }
}

void Private::Presentation::readSymbols(QXmlStreamReader& reader) {
  while (reader.readNextStartElement()) {
    Q_ASSERT(reader.name() == "symbol");

    QString symbolName;
    int id = reader.attributes().value("RCID").toInt();
    S52::ModelData dv;
    S52::ModelData dr;
    QString description;
    S52::ColorRef ref;
    QString instruction;
    QPoint gref;
    bool raster;

    while (reader.readNextStartElement()) {
      if (reader.name() == "name") {
        symbolName = reader.readElementText();
      } else if (reader.name() == "definition") {
        raster = reader.readElementText() == "R";
      } else if (reader.name() == "vector") {
        dv = parseModelData(reader, nullptr, &instruction);
      } else if (reader.name() == "bitmap") {
        dr = parseModelData(reader, &gref, nullptr);
      } else if (reader.name() == "description") {
        description = reader.readElementText();
      } else if (reader.name() == "color-ref") {
        ref = parseColorRef(reader);
      } else if (reader.name() == "prefer-bitmap") {
        reader.skipCurrentElement();
      } else {
        qWarning() << "Don't know how to handle" << reader.name();
        reader.skipCurrentElement();
      }
    }
    if (names.contains(symbolName) && symbols.contains(names[symbolName])) {
      if (!symbols[names[symbolName]].isRaster() || raster) {
        qWarning() << symbolName << "already parsed, skipping";
        continue;
      }
    }

    if (!names.contains(symbolName)) {
      names[symbolName] = m_nextSymbolIndex++;
    }
    if (raster) {
      QString src = QString("%1 %2").arg(gref.x()).arg(gref.y());
      symbols[names[symbolName]] = S52::Symbol(id, dr, description, ref, src, raster);
    } else {
      symbols[names[symbolName]] = S52::Symbol(id, dv, description, ref, instruction, raster);
    }
  }
}

void Private::Presentation::init() {

  functions = new S52::Functions();

  for (S52::Lookup* lup: lookups) {
    int err = parseInstruction(lup);
    if (err != 0) {
      qWarning() << "Error parsing" << lup->source();
    }
  }
}
