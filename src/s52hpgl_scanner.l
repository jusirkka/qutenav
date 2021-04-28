/* -*- coding: utf-8-unix -*-
 *
 * File: src/s52hpgl_scanner.l
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

#include "hpglparser.h"
#define YY_NO_UNPUT
#include "s52hpgl_parser.h"

#ifndef YYSTYPE
#define YYSTYPE S52HPGL_STYPE
#endif
#ifndef YYLTYPE
#define YYLTYPE S52HPGL_LTYPE
#endif

#define YY_USER_ACTION  \
    yylloc->pos = yylloc->prev_pos; yylloc->prev_pos += yyleng;

#define YY_USER_INIT  \
    yylloc->prev_pos = 0;

#define YY_DECL int s52hpgl_lex(YYSTYPE* yylval_param, YYLTYPE* yylloc_param, yyscan_t yyscanner)

%}

%option reentrant
%option bison-locations
%option noyywrap
%option prefix="s52hpgl_"



dig            [0-9]
num             [-+]?{dig}+

singlechar     [;,]

/* ascii >= 64 says the spec */
colorname      [@A-Z\[\\\]\^_\`a-z\{\|\}~]

%%

{num}              yylval->v_int = static_cast<int>(strtol(yytext, NULL, 0)); return INT;

"SP"                return SP;
"ST"                return ST;
"SW"                return SW;
"PU"                return PU;
"PD"                return PD;
"CI"                return CI;
"PM"                return PM;
"EP"                return EP;
"FP"                return FP;
"AA"                return AA;

{colorname}         yylval->v_char = *yytext; return COLOR;
{singlechar}        yylval->v_char = *yytext; return *yytext;



%%

