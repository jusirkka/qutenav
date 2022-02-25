/* -*- coding: utf-8-unix -*-
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

#pragma once

#include <QXmlStreamReader>
#include <QHash>
#include "s52presentation.h"
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


  quint32 m_nextSymbolIndex;

private slots:

  void setColorTable(quint8 t);

public:

  void init();
  int parseInstruction(S52::Lookup* lup);

  S52::Lookup::Type typeFilter(const S57::Object* obj) const;

  using IdentifierHash = QHash<QString, quint32>;

  IdentifierHash names;


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
};

} // namespace Private



void s52instr_error(Private::LocationType*,
                    Private::Presentation*,
                    S52::Lookup*,
                    yyscan_t, const char*);

