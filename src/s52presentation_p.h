#pragma once
#include <QXmlStreamReader>
#include <QHash>
#include "s52presentation.h"
#include "settings.h"
#include "types.h"

#define S52INSTR_LTYPE Private::LocationType
#define S52INSTR_STYPE Private::ValueType

#ifndef YY_TYPEDEF_YY_SCANNER_T
#define YY_TYPEDEF_YY_SCANNER_T
using yyscan_t = void *;
#endif

#define YYLLOC_DEFAULT(Current, Rhs, N) do if (N) {\
    (Current).pos = YYRHSLOC (Rhs, 1).pos;\
    (Current).prev_pos = YYRHSLOC (Rhs, 1).prev_pos;\
    } else {                                        \
    (Current).pos = YYRHSLOC (Rhs, 0).pos;\
    (Current).prev_pos = YYRHSLOC (Rhs, 0).prev_pos;\
    } while (0)

namespace Private {

// flex/bison location and value types
struct LocationType {
  int prev_pos;
  int pos;
};

struct ValueType {
  QString v_string;
  char v_char;
  int v_int;
  float v_float;
};


class Presentation: public QObject {

  Q_OBJECT

public:

  static Presentation* instance();

private:

  Presentation();

  void readObjectClasses();
  void readAttributes();
  void readChartSymbols();

  void readColorTables(QXmlStreamReader& reader);
  void readLookups(QXmlStreamReader& reader);
  void readSymbolNames(QXmlStreamReader& reader);

  int parseInstruction(S52::Lookup* lup);

  quint32 m_nextSymbolIndex;

private slots:

  void setSimplifiedSymbols(bool);
  void setPlainBoundaries(bool);
  void setColorTable(Settings::ColorTable t);

public:

  void init();

  S52::Lookup::Type typeFilter(const S57::Object* obj) const;

  using IdentifierHash = QHash<QString, quint32>;

  struct ClassDescription {
    ClassDescription(const QString& c, const QString& d, bool meta)
      : code(c)
      , description(d)
      , isMeta(meta) {}

    ClassDescription() = default;

    QString code;
    QString description;
    bool isMeta;
  };

  using ClassHash = QHash<quint32, ClassDescription>;

  IdentifierHash names;
  ClassHash classes;

  using DescriptionMap = QMap<quint32, QString>;

  struct AttributeDescription {
    AttributeDescription(const QString& c, S57::Attribute::Type t, const QString& d)
      : code(c)
      , type(t)
      , description(d) {}

    AttributeDescription() = default;

    QString code;
    S57::Attribute::Type type;
    QString description;
    DescriptionMap enumDescriptions;
  };
  using AttributeMap = QMap<quint32, AttributeDescription>;

  AttributeMap attributes;


  struct SymbolDescription {
    SymbolDescription(const QString& c, const QString& d)
      : code(c)
      , description(d) {}

    SymbolDescription() = default;

    QString code;
    QString description;
  };

  using SymbolHash = QHash<SymbolKey, SymbolDescription>;

  SymbolHash symbols;

  using ColorVector = QVector<QColor>;

  struct ColorTable {
    ColorTable(const QString& gfile)
      : graphicsFile(gfile) {}

    ColorTable() = default;

    QString graphicsFile;
    ColorVector colors;
  };

  QVector<ColorTable> colorTables;
  quint32 currentColorTable;


  using LookupVector = QVector<S52::Lookup*>;
  using LookupHash = QHash<quint32, LookupVector>; // key: class code
  using LookupTable = QMap<S52::Lookup::Type, LookupHash>;

  using LUPTableIterator = QMap<S52::Lookup::Type, LookupHash>::const_iterator;
  using LUPHashIterator = QHash<quint32, LookupVector>::const_iterator;

  LookupTable lookupTable;

  S52::Functions* functions;
  Settings* settings;
  bool simplifiedSymbols;
  bool plainBoundaries;
};

} // namespace Private



void s52instr_error(Private::LocationType*,
                    Private::Presentation*,
                    S52::Lookup*,
                    yyscan_t, const char*);

