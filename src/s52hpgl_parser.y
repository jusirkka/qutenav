%{

#include "s52presentation_p.h"

int s52hpgl_lex(Private::HPGL_ValueType*, Private::LocationType*, yyscan_t);


%}

%locations
%define parse.trace

%define api.prefix {s52hpgl_}
%define api.pure full
%define api.value.type {Private::HPGL_ValueType}

%parse-param {Private::Presentation* reader}
%parse-param {S52::LineStyle* symbol}
%parse-param {yyscan_t scanner}
%lex-param {yyscan_t scanner}

%token SP ST SW PU PD CI PM EP FP

%token <v_int> INT
%token <v_char> COLOR


// Grammar follows

%%

input:
  commands
  ;

commands:
  command ';' | commands command ';'
  ;


command: SP COLOR
  ;

command: ST INT
  ;

command: SW INT
  ;

command: PU points
  ;

command: PD points
  ;

command: CI INT
  ;

command: PM INT
  ;

command: EP
  ;

command: FP
  ;

points: INT | points ',' INT
  ;

