/* -*- coding: utf-8-unix -*-
 *
 * File: src/s52instr_scanner.l
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
#define YY_NO_UNPUT
#include "s52instr_parser.h"

#ifndef YYSTYPE
#define YYSTYPE S52INSTR_STYPE
#endif
#ifndef YYLTYPE
#define YYLTYPE S52INSTR_LTYPE
#endif


#define YY_USER_ACTION  \
    yylloc->pos = yylloc->prev_pos; yylloc->prev_pos += yyleng;

#define YY_USER_INIT  \
    yylloc->prev_pos = 0;

#define YY_DECL int s52instr_lex(YYSTYPE* yylval_param, YYLTYPE* yylloc_param, yyscan_t yyscanner)

%}

%option reentrant
%option bison-locations
%option noyywrap
%option prefix="s52instr_"



dig             [0-9]
unum            {dig}+
num             [-+]?{dig}+
float_1         {num}\.({unum})?([Ee]{num})?
float_2         \.{unum}([Ee]{num})?
float_3         {num}[Ee]{num}

float           {float_1}|{float_2}|{float_3}
singlechar      [\(\);,=]

classname      [A-Za-z$_][A-Za-z$_0-9]{5}
symbolname     {classname}[A-Za-z$_0-9][0-9]
colorname      [A-Za-z$][A-Za-z$_0-9]{4}
charspec       \'[1-2][4-6][0-2][0-9]+\'

%x INSTRING

%%

"WRECKS02"          yylval->v_string = yytext; return OVERLING;
"OBSTRN04"          yylval->v_string = yytext; return OVERLING;
"DEPCNT02"          yylval->v_string = yytext; return OVERRIDER;

{symbolname}        yylval->v_string = yytext; return SYMBOL;
{classname}         yylval->v_string = yytext; return VARIABLE;
{colorname}         yylval->v_string = yytext; return COLOR;

{float}             yylval->v_float = strtof(yytext, NULL); return FLOAT;
{num}               yylval->v_int = static_cast<int>(strtol(yytext, NULL, 0)); return INT;

{charspec}          {
                      QString s = yytext;
                      yylval->v_string = s.mid(1, s.length() - 2);
                      return CHARSPEC;
                    }

"TX"                return TX;
"TE"                return TE;
"SY"                return SY;
"LS"                return LS;
"LC"                return LC;
"AC"                return AC;
"AP"                return AP;
"CS"                return CS;

"SOLD"              return SOLID;
"DASH"              return DASHED;
"DOTT"              return DOTTED;

{singlechar}        yylval->v_char = *yytext; return *yytext;

\'                 BEGIN(INSTRING); return BEGINSTRING;
<INSTRING>\'       BEGIN(0); return ENDSTRING;
<INSTRING>.        yylval->v_char = *yytext; return CHAR;

%%

