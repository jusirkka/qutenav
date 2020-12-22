%{

#include "hpglparser.h"
#include <QDebug>

int s52hpgl_lex(HPGLParser::ValueType*, HPGLParser::LocationType*, yyscan_t);

class HPGLHelper {
public:
  inline void setColor(HPGLParser* p, char c) {
    p->setColor(c);
  }
  inline void setAlpha(HPGLParser* p, int a) {
    p->setAlpha(a);
  }
  inline void setWidth(HPGLParser* p, int w) {
    p->setWidth(w);
  }
  inline void movePen(HPGLParser* p, const QVector<int>& ps) {
    p->movePen(ps);
  }
  inline void drawLineString(HPGLParser* p, const QVector<int>& ps) {
    p->drawLineString(ps);
  }
  inline void drawCircle(HPGLParser* p, int r) {
    p->drawCircle(r);
  }
  inline void drawArc(HPGLParser* p, int x, int y, int a) {
    p->drawArc(x, y, a);
  }
  inline void pushSketch(HPGLParser* p) {
    p->pushSketch();
  }
  inline void endSketch(HPGLParser* p) {
    p->endSketch();
  }
  inline void fillSketch(HPGLParser* p) {
    p->fillSketch();
  }
  inline void edgeSketch(HPGLParser* p) {
    p->edgeSketch();
  }
  inline void setError(HPGLParser* p) {
    p->m_ok = false;
  }
};

#define ABORT(p, sc) {s52hpgl_error(nullptr, p, sc, "Parse Error"); YYERROR;}

%}

%locations
%define parse.trace

%define api.prefix {s52hpgl_}
%define api.pure full
%define api.value.type {HPGLParser::ValueType}

%parse-param {HPGLParser* parent}
%parse-param {yyscan_t scanner}
%lex-param {yyscan_t scanner}

%token SP ST SW PU PD CI PM EP FP AA

%token <v_int> INT
%token <v_char> COLOR

%type <v_int_list> points


// Grammar follows

%%

input:
  commands
  ;

commands:
  command ';' | commands command ';'
  ;


command: SP COLOR {
  HPGLHelper h;
  h.setColor(parent, $2);
  if (!parent->ok()) ABORT(parent, scanner);
};

command: ST INT {
  HPGLHelper h;
  h.setAlpha(parent, $2);
  if (!parent->ok()) ABORT(parent, scanner);
};

command: SW INT {
  HPGLHelper h;
  h.setWidth(parent, $2);
  if (!parent->ok()) ABORT(parent, scanner);
};

command: PU points {
  HPGLHelper h;
  h.movePen(parent, $2);
  if (!parent->ok()) ABORT(parent, scanner);
};

command: PD points {
  HPGLHelper h;
  h.drawLineString(parent, $2);
  if (!parent->ok()) ABORT(parent, scanner);
};

command: PD /*empty*/ {
  HPGLHelper h;
  h.drawLineString(parent, HPGLParser::RawPoints());
  if (!parent->ok()) ABORT(parent, scanner);
};

command: CI INT {
  HPGLHelper h;
  h.drawCircle(parent, $2);
  if (!parent->ok()) ABORT(parent, scanner);
};

command: PM INT {
  HPGLHelper h;
  if ($2 == 0) {
    h.pushSketch(parent);
  } else if ($2 == 2) {
    h.endSketch(parent);
  } else {
    qWarning() << "PM: unsupported parameter" << $2;
    h.setError(parent);
  }
  if (!parent->ok()) ABORT(parent, scanner);
};

command: EP {
  HPGLHelper h;
  h.edgeSketch(parent);
  if (!parent->ok()) ABORT(parent, scanner);
};

command: FP {
  HPGLHelper h;
  h.fillSketch(parent);
  if (!parent->ok()) ABORT(parent, scanner);
};

command: AA INT ',' INT ',' INT {
  HPGLHelper h;
  h.drawArc(parent, $2, $4, $6);
  if (!parent->ok()) ABORT(parent, scanner);
};

points: INT {
  $$.clear();
  $$.append($1);
};

points: points ',' INT {
  $$.append($3);
};

