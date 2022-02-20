/* -*- coding: utf-8-unix -*-
 *
 * File: src/s52instr_parser.y
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
%{

#include "s52presentation_p.h"
#include "logging.h"
#include "types.h"

int s52instr_lex(Private::ValueType*, Private::LocationType*, yyscan_t);

namespace S52 {
class ByteCoder {
public:
  void setCode(S52::Lookup* lup, S52::Lookup::Code c) {
    lup->m_code.append(c);
  }
  void setImmed(S52::Lookup* lup, const QVariant& v) {
    lup->m_immed.append(v);
  }
  void setRef(S52::Lookup* lup, quint32 v) {
    lup->m_references.append(v);
  }
  void clear(S52::Lookup* lup) {
    lup->m_code.clear();
    lup->m_references.clear();
    lup->m_immed.clear();
  }
  void needsUnderling(S52::Lookup* lup) {
    lup->m_needUnderling = true;
  }
  void canOverride(S52::Lookup* lup) {
    lup->m_canOverride = true;
  }
};
}

#define ABORT do {S52::ByteCoder bc; bc.clear(lookup); YYERROR;} while (false)

%}

%locations
%define parse.trace

%define api.prefix {s52instr_}
%define api.pure full
%define api.value.type {Private::ValueType}

%parse-param {Private::Presentation* reader}
%parse-param {S52::Lookup* lookup}
%parse-param {yyscan_t scanner}
%lex-param {yyscan_t scanner}

%token TX TE SY LS LC AC AP CS
%token SOLID DASHED DOTTED BEGINSTRING ENDSTRING

%token <v_char> CHAR
%token <v_int> INT
%token <v_float> FLOAT
%token <v_string> SYMBOL VARIABLE COLOR CHARSPEC OVERLING OVERRIDER

%type <v_int> opttransparency varstring varint
%type <v_int> pstyle
%type <v_string> string chars optrotation

// Grammar follows


%%

input: commands;

commands: command | commands ';' command;

command: TX '(' varstring ',' varint ',' varint ',' varint ',' CHARSPEC ','
         INT ',' INT ',' COLOR ',' INT ')' {
  auto fun = S52::FindFunction("TX");
  bool ok = $3 && $5 && $7 && $9 && reader->names.contains($17);
  if (ok) {
    S52::ByteCoder bc;

    // charspec
    bc.setCode(lookup, S52::Lookup::Code::Immed);
    bc.setImmed(lookup, QVariant::fromValue($11));
    // x-offset
    bc.setCode(lookup, S52::Lookup::Code::Immed);
    bc.setImmed(lookup, QVariant::fromValue($13));
    // y-offset
    bc.setCode(lookup, S52::Lookup::Code::Immed);
    bc.setImmed(lookup, QVariant::fromValue($15));
    // color
    bc.setCode(lookup, S52::Lookup::Code::Immed);
    bc.setImmed(lookup, QVariant::fromValue(reader->names[$17]));
    // display
    bc.setCode(lookup, S52::Lookup::Code::Immed);
    bc.setImmed(lookup, QVariant::fromValue($19));

    bc.setCode(lookup, S52::Lookup::Code::Fun);
    bc.setRef(lookup, fun->index());
  } else {
    qCWarning(CS52) << "TX: argument parsing error";
    ABORT;
  }
};


command: TE '(' string ',' string ',' INT ',' INT ',' INT ',' CHARSPEC ','
         INT ',' INT ',' COLOR ',' INT ')' {
  auto fun = S52::FindFunction("TE");
  bool ok = reader->names.contains($19);
  QVector<quint32> refs;
  if (ok) {
    QStringList attrs = $5.split(",");
    for (const QString& attr: attrs) {
      if (!reader->names.contains(attr)) {
        ok = false;
        break;
      }
      refs.append(reader->names[attr]);
    }
  }
  if (ok) {
    S52::ByteCoder bc;

    // format
    bc.setCode(lookup, S52::Lookup::Code::Immed);
    bc.setImmed(lookup, QVariant::fromValue($3));
    // number of attributes
    bc.setCode(lookup, S52::Lookup::Code::Immed);
    bc.setImmed(lookup, QVariant::fromValue(refs.size()));
    // attribute references
    for (quint32 ref: refs) {
      bc.setCode(lookup, S52::Lookup::Code::Var);
      bc.setRef(lookup, ref);
    }
    // hjust
    bc.setCode(lookup, S52::Lookup::Code::Immed);
    bc.setImmed(lookup, QVariant::fromValue($7));
    // vjust
    bc.setCode(lookup, S52::Lookup::Code::Immed);
    bc.setImmed(lookup, QVariant::fromValue($9));
    // space
    bc.setCode(lookup, S52::Lookup::Code::Immed);
    bc.setImmed(lookup, QVariant::fromValue($11));
    // charspec
    bc.setCode(lookup, S52::Lookup::Code::Immed);
    bc.setImmed(lookup, QVariant::fromValue($13));
    // x-offset
    bc.setCode(lookup, S52::Lookup::Code::Immed);
    bc.setImmed(lookup, QVariant::fromValue($15));
    // y-offset
    bc.setCode(lookup, S52::Lookup::Code::Immed);
    bc.setImmed(lookup, QVariant::fromValue($17));
    // color
    bc.setCode(lookup, S52::Lookup::Code::Immed);
    bc.setImmed(lookup, QVariant::fromValue(reader->names[$19]));
    // display
    bc.setCode(lookup, S52::Lookup::Code::Immed);
    bc.setImmed(lookup, QVariant::fromValue($21));

    bc.setCode(lookup, S52::Lookup::Code::Fun);
    bc.setRef(lookup, fun->index());
  } else {
    qCWarning(CS52) << "TE: argument parsing error";
    ABORT;
  }

};



varstring: VARIABLE {
  $$ = reader->names.contains($1);
  if ($$) {
    S52::ByteCoder bc;

    // attribute value is interpreted as string in TX procedure
    bc.setCode(lookup, S52::Lookup::Code::Immed);
    bc.setImmed(lookup, QVariant::fromValue(reader->names[$1]));
    // qDebug() << $1 << reader->names[$1];
  } else {
    qCWarning(CS52) << "TX: attribute not found" << $1;
  }
};

varstring: string {
  S52::ByteCoder bc;
  bc.setCode(lookup, S52::Lookup::Code::Immed);
  bc.setImmed(lookup, QVariant::fromValue($1));
  $$ = true;
};

string: BEGINSTRING chars ENDSTRING {
  $$ = $2;
};

chars: CHAR {
  $$ = QString($1);
};

chars: chars CHAR {
  $$.append($2);
};

varint: INT {
  S52::ByteCoder bc;
  bc.setCode(lookup, S52::Lookup::Code::Immed);
  bc.setImmed(lookup, QVariant::fromValue($1));
  $$ = true;
};

varint: VARIABLE '=' INT {
  $$ = reader->names.contains($1);
  if ($$) {
    S52::ByteCoder bc;

    bc.setCode(lookup, S52::Lookup::Code::DefVar);
    bc.setRef(lookup, reader->names[$1]);
    bc.setImmed(lookup, QVariant::fromValue($3));
    // qDebug() << $1 << reader->names[$1];
  } else {
    qCWarning(CS52) << "TX: attribute not found" << $1;
  }
};


command: SY '(' SYMBOL optrotation ')' {
  auto fun = S52::FindFunction("SY");
  bool ok = reader->names.contains($3);
  bool lit;
  float rot = $4.toFloat(&lit);

  bool rok = lit ? true : reader->names.contains($4);

  if (rok) {
    S52::ByteCoder bc;

    bc.setCode(lookup, S52::Lookup::Code::Immed);
    if (ok) {
      bc.setImmed(lookup, QVariant::fromValue(reader->names[$3]));
    } else {
      qCWarning(CS52PS) << "SY: unknown symbol" << $3;
      bc.setImmed(lookup, QVariant::fromValue(reader->names["QUESMRK1"]));
    }

    if (lit) {
      bc.setCode(lookup, S52::Lookup::Code::Immed);
      bc.setImmed(lookup, QVariant::fromValue(rot));
    } else {
      bc.setCode(lookup, S52::Lookup::Code::Var);
      bc.setRef(lookup, reader->names[$4]);
    }

    bc.setCode(lookup, S52::Lookup::Code::Fun);
    bc.setRef(lookup, fun->index());
  } else {
    qCWarning(CS52) << "SY: unknown attribute" << $4;
    ABORT;
  }
};

optrotation: %empty {
  $$ = "0.";
}

optrotation: ',' INT {
  $$ = QString::number($2);
};

optrotation: ',' FLOAT {
  $$ = QString::number($2);
};

optrotation: ',' VARIABLE {
  $$ = $2;
};

command: LS '(' pstyle ',' INT ',' COLOR ')' {
  auto fun = S52::FindFunction("LS");
  bool ok = reader->names.contains($7);
  if (ok) {
    S52::ByteCoder bc;

    bc.setCode(lookup, S52::Lookup::Code::Immed);
    bc.setImmed(lookup, QVariant::fromValue($3));

    bc.setCode(lookup, S52::Lookup::Code::Immed);
    bc.setImmed(lookup, QVariant::fromValue($5));

    bc.setCode(lookup, S52::Lookup::Code::Immed);
    bc.setImmed(lookup, QVariant::fromValue(reader->names[$7]));

    bc.setCode(lookup, S52::Lookup::Code::Fun);
    bc.setRef(lookup, fun->index());
  } else {
    qCWarning(CS52) << "LS: unknown color" << $7;
    ABORT;
  }
};

pstyle: SOLID {
  $$ = as_numeric(S52::LineType::Solid);
};

pstyle: DASHED {
  $$ = as_numeric(S52::LineType::Dashed);
};

pstyle: DOTTED {
  $$ = as_numeric(S52::LineType::Dotted);
};

command: LC '(' SYMBOL ')' {
  auto fun = S52::FindFunction("LC");
  bool ok = reader->names.contains($3);

  S52::ByteCoder bc;

  bc.setCode(lookup, S52::Lookup::Code::Immed);
  if (ok) {
    bc.setImmed(lookup, QVariant::fromValue(reader->names[$3]));
  } else {
    qCWarning(CS52PS) << "LC: unknown symbol" << $3;
    bc.setImmed(lookup, QVariant::fromValue(reader->names["QUESMRK1"]));
  }

  bc.setCode(lookup, S52::Lookup::Code::Fun);
  bc.setRef(lookup, fun->index());
};

command: AC '(' COLOR opttransparency ')' {
  auto fun = S52::FindFunction("AC");
  bool ok = reader->names.contains($3);
  if (ok) {
    S52::ByteCoder bc;

    bc.setCode(lookup, S52::Lookup::Code::Immed);
    bc.setImmed(lookup, QVariant::fromValue(reader->names[$3]));

    bc.setCode(lookup, S52::Lookup::Code::Immed);
    bc.setImmed(lookup, QVariant::fromValue($4));

    bc.setCode(lookup, S52::Lookup::Code::Fun);
    bc.setRef(lookup, fun->index());
  } else {
    qCWarning(CS52) << "AC: unknown color" << $3;
    ABORT;
  }
};

opttransparency: %empty {
  $$ = 255;
};

opttransparency: ',' INT {
  $$ = 255 - $2 * 255 / 4;
};

command: AP '(' SYMBOL optrotation ')' {
  auto fun = S52::FindFunction("AP");
  bool ok = reader->names.contains($3);
  bool lit;
  float rot = $4.toFloat(&lit);

  bool rok = lit ? true : reader->names.contains($4);

  if (rok) {
    S52::ByteCoder bc;

    bc.setCode(lookup, S52::Lookup::Code::Immed);
    if (ok) {
      bc.setImmed(lookup, QVariant::fromValue(reader->names[$3]));
    } else {
      qCWarning(CS52PS) << "AP: unknown symbol" << $3;
      bc.setImmed(lookup, QVariant::fromValue(reader->names["QUESMRK1"]));
    }

    if (lit) {
      bc.setCode(lookup, S52::Lookup::Code::Immed);
      bc.setImmed(lookup, QVariant::fromValue(rot));
    } else {
      bc.setCode(lookup, S52::Lookup::Code::Var);
      bc.setRef(lookup, reader->names[$4]);
    }

    bc.setCode(lookup, S52::Lookup::Code::Fun);
    bc.setRef(lookup, fun->index());
  } else {
    qCWarning(CS52) << "AP: unknown attribute" << $4;
    ABORT;
  }
};

command: CS '(' OVERLING ')' {
  auto fun = S52::FindFunction($3);
  if (fun != nullptr) {
    S52::ByteCoder bc;

    bc.needsUnderling(lookup);
    bc.canOverride(lookup);

    bc.setCode(lookup, S52::Lookup::Code::Fun);
    bc.setRef(lookup, fun->index());
  } else {
    qCWarning(CS52) << "CS: unknown procedure" << $3;
    ABORT;
  }
};

command: CS '(' OVERRIDER ')' {
  auto fun = S52::FindFunction($3);
  if (fun != nullptr) {
    S52::ByteCoder bc;

    bc.canOverride(lookup);

    bc.setCode(lookup, S52::Lookup::Code::Fun);
    bc.setRef(lookup, fun->index());
  } else {
    qCWarning(CS52) << "CS: unknown procedure" << $3;
    ABORT;
  }
};


command: CS '(' SYMBOL ')' {
  auto fun = S52::FindFunction($3);
  if (fun != nullptr) {
    S52::ByteCoder bc;

    bc.setCode(lookup, S52::Lookup::Code::Fun);
    bc.setRef(lookup, fun->index());
  } else {
    qCWarning(CS52) << "CS: unknown procedure" << $3;
    ABORT;
  }
};

