%{

#include "s52presentation_p.h"
#include <QDebug>

int s52instr_lex(Private::Instr_ValueType*, Private::LocationType*, yyscan_t);

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
};
}

%}

%locations
%define parse.trace

%define api.prefix {s52instr_}
%define api.pure full
%define api.value.type {Private::Instr_ValueType}

%parse-param {Private::Presentation* reader}
%parse-param {S52::Lookup* lookup}
%parse-param {yyscan_t scanner}
%lex-param {yyscan_t scanner}

%token TX TE SY LS LC AC AP CS
%token SOLID DASHED DOTTED BEGINSTRING ENDSTRING

%token <v_char> CHAR
%token <v_int> INT
%token <v_float> FLOAT
%token <v_string> SYMBOL VARIABLE COLOR CHARSPEC

%type <v_int> opttransparency varstring
%type <v_float> optrotation
%type <v_char> pstyle
%type <v_string> string chars

// Grammar follows


%%

input: commands;

commands: command | commands ';' command;

command: TX '(' varstring ',' INT ',' INT ',' INT ',' CHARSPEC ','
         INT ',' INT ',' COLOR ',' INT ')' {
  auto fun = S52::FindFunction("TX");
  bool ok = $3 && fun != nullptr && reader->names.contains($17);
  if (ok) {
    S52::ByteCoder bc;

    // hjust
    bc.setCode(lookup, S52::Lookup::Code::Immed);
    bc.setImmed(lookup, QVariant::fromValue($5));
    // vjust
    bc.setCode(lookup, S52::Lookup::Code::Immed);
    bc.setImmed(lookup, QVariant::fromValue($7));
    // space
    bc.setCode(lookup, S52::Lookup::Code::Immed);
    bc.setImmed(lookup, QVariant::fromValue($9));
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
    qWarning() << "TX: parse error, called with (attribute)" << $5 << $7 << $9 << $11 << $13 << $15 << $17 << $19;
  }
};


command: TE '(' string ',' string ',' INT ',' INT ',' INT ',' CHARSPEC ','
         INT ',' INT ',' COLOR ',' INT ')' {
  auto fun = S52::FindFunction("TE");
  bool ok = fun != nullptr && reader->names.contains($19);
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
    qWarning() << "TE: parse error, called with" << $3 << $5 << $7 << $9 << $11 << $13 << $15 << $17 << $19 << $21;
  }

};



varstring: VARIABLE {
  $$ = reader->names.contains($1);
  if ($$) {
    S52::ByteCoder bc;

    bc.setCode(lookup, S52::Lookup::Code::Var);
    bc.setRef(lookup, reader->names[$1]);
  } else {
    qWarning() << "TX: attribute not found" << $1;
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

command: SY '(' SYMBOL optrotation ')' {
  auto fun = S52::FindFunction("SY");
  bool ok = reader->names.contains($3) && fun != nullptr;
  if (ok) {
    S52::ByteCoder bc;

    bc.setCode(lookup, S52::Lookup::Code::Immed);
    bc.setImmed(lookup, QVariant::fromValue(reader->names[$3]));

    bc.setCode(lookup, S52::Lookup::Code::Immed);
    bc.setImmed(lookup, QVariant::fromValue($4));

    bc.setCode(lookup, S52::Lookup::Code::Fun);
    bc.setRef(lookup, fun->index());
  } else {
    qWarning() << "SY: parse error, called with " << $3 << $4;
  }
};

optrotation: %empty {
  $$ = 0.;
}

optrotation: ',' INT {
  $$ = $2;
};

optrotation: ',' FLOAT {
  $$ = $2;
};

command: LS '(' pstyle ',' INT ',' COLOR ')' {
  auto fun = S52::FindFunction("LS");
  bool ok = reader->names.contains($7) && fun != nullptr;
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
    qWarning() << "LS: parse error, called with " << $3 << $5 << $7;
  }
};

pstyle: SOLID {
  $$ = char(S52::Lookup::Line::Solid);
};

pstyle: DASHED {
  $$ = char(S52::Lookup::Line::Dashed);
};

pstyle: DOTTED {
  $$ = char(S52::Lookup::Line::Dotted);
};

command: LC '(' SYMBOL ')' {
  auto fun = S52::FindFunction("LC");
  bool ok = reader->names.contains($3) && fun != nullptr;
  if (ok) {
    S52::ByteCoder bc;

    bc.setCode(lookup, S52::Lookup::Code::Immed);
    bc.setImmed(lookup, QVariant::fromValue(reader->names[$3]));

    bc.setCode(lookup, S52::Lookup::Code::Fun);
    bc.setRef(lookup, fun->index());
  } else {
    qWarning() << "LC: parse error, called with " << $3;
  }
};

command: AC '(' COLOR opttransparency ')' {
  auto fun = S52::FindFunction("AC");
  bool ok = reader->names.contains($3) && fun != nullptr;
  if (ok) {
    S52::ByteCoder bc;

    bc.setCode(lookup, S52::Lookup::Code::Immed);
    bc.setImmed(lookup, QVariant::fromValue(reader->names[$3]));

    bc.setCode(lookup, S52::Lookup::Code::Immed);
    bc.setImmed(lookup, QVariant::fromValue($4));

    bc.setCode(lookup, S52::Lookup::Code::Fun);
    bc.setRef(lookup, fun->index());
  } else {
    qWarning() << "AC: parse error, called with " << $3 << $4;
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
  bool ok = reader->names.contains($3) && fun != nullptr;
  if (ok) {
    S52::ByteCoder bc;

    bc.setCode(lookup, S52::Lookup::Code::Immed);
    bc.setImmed(lookup, QVariant::fromValue(reader->names[$3]));

    bc.setCode(lookup, S52::Lookup::Code::Immed);
    bc.setImmed(lookup, QVariant::fromValue($4));

    bc.setCode(lookup, S52::Lookup::Code::Fun);
    bc.setRef(lookup, fun->index());
  } else {
    qWarning() << "AP: parse error, called with " << $3 << $4;
  }
};


command: CS '(' SYMBOL ')' {
  auto fun = S52::FindFunction($3);
  if (fun != nullptr) {
    S52::ByteCoder bc;

    bc.setCode(lookup, S52::Lookup::Code::Fun);
    bc.setRef(lookup, fun->index());
  } else {
    qWarning() << "CS: parse error, called with " << $3;
  }
};

