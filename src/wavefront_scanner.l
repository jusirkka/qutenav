/* -*- coding: utf-8-unix -*-
 *
 * File: src/wavefront_scanner.l
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

#include "wfreader.h"
#define YY_NO_UNPUT
#include "wavefront_parser.h"
#ifndef YYSTYPE
#define YYSTYPE WAVEFRONT_STYPE
#endif
#ifndef YYLTYPE
#define YYLTYPE WAVEFRONT_LTYPE
#endif

#define YY_USER_ACTION  \
    yylloc->col = yylloc->prev_col; yylloc->prev_col += yyleng; yylloc->pos = yylloc->prev_pos; yylloc->prev_pos += yyleng;

#define YY_USER_INIT  \
    yylloc->prev_col = 0; yylloc->row = 0; yylloc->prev_pos = 0;

#define my_init_line \
    yylloc->row++; yylloc->prev_col = 0;

#define YY_DECL int wavefront_lex(YYSTYPE* yylval_param, YYLTYPE* yylloc_param, yyscan_t yyscanner)

%}

%option reentrant
%option bison-locations
%option noyywrap
%option prefix="wavefront_"



dig             [0-9]
unum            {dig}+
num             [-+]?{dig}+
float_1         {num}\.({unum})?([Ee]{num})?
float_2         \.{unum}([Ee]{num})?
float_3         {num}[Ee]{num}

triple_v        {unum}\/\/
triple_vt       {unum}\/{unum}\/
triple_vn       {unum}\/\/{unum}
triple_vtn      {unum}\/{unum}\/{unum}


space           [ \t]

unsupp1         vp|bmat|step|p|curv|curv2|trom|hole|scrv|sp
unsupp2         con|g|s|mg|o|bevel|c_interp|d_interp|lod|usemtl|mtllib|shadow_obj
unsupp3         trace_obj|ctech|stech|call|csh

unsupp          {unsupp1}|{unsupp2}|{unsupp3}

normal          vn
texcoord        vt
cstype          cstype
deg             deg
surf            surf
parm            parm
end             end

typename        bezier


v_or_vertex     v
face            f
varname_u       u
line            l


%%


{texcoord}          return TEXCOORD;
{normal}            return NORMAL;
{cstype}            return CSTYPE;
{deg}               return DEG;
{surf}              return SURF;
{parm}              return PARM;
{end}               return SURFEND;
{unsupp}            return UNSUPP;
{v_or_vertex}       return V_OR_VERTEX;
{face}              return FACE;
{line}              return LINE_STRIP;

{varname_u}         return U;

{typename}          {
                        yylval->v_string = yytext;
                        return TYPENAME;
                    }

{triple_v}          {
                        char* first = index(yytext, '/');
                        yylval->v_triplet[0] = strtof(yytext, &first);
                        yylval->v_triplet[1] = 0;
                        yylval->v_triplet[2] = 0;
                        return VERT;
                    }

{triple_vt}         {
                        char* first = index(yytext, '/');
                        char* last = rindex(yytext, '/');
                        yylval->v_triplet[0] = strtof(yytext, &first);
                        yylval->v_triplet[1] = strtof(first + 1, &last);
                        yylval->v_triplet[2] = 0;
                        return VERT;
                    }

{triple_vn}         {
                        char* first = index(yytext, '/');
                        char* last = rindex(yytext, '/');
                        yylval->v_triplet[0] = strtof(yytext, &first);
                        yylval->v_triplet[1] = 0;
                        yylval->v_triplet[2] = strtof(last + 1, NULL);
                        return VERT;
                    }

{triple_vtn}        {
                        char* first = index(yytext, '/');
                        char* last = rindex(yytext, '/');
                        yylval->v_triplet[0] = strtof(yytext, &first);
                        yylval->v_triplet[1] = strtof(first + 1, &last);
                        yylval->v_triplet[2] = strtof(last + 1, NULL);
                        return VERT;
                    }

{float_1}           yylval->v_float = strtod(yytext, NULL); return FLOAT;
{float_2}           yylval->v_float = strtod(yytext, NULL); return FLOAT;
{float_3}           yylval->v_float = strtod(yytext, NULL); return FLOAT;
{num}               yylval->v_int = static_cast<int>(strtol(yytext, NULL, 0)); return INT;




"#".*\n             my_init_line; // eat up comment line

^{space}*\n         my_init_line; // eat up empty line

\\\n                my_init_line; // line continuation

\n                  my_init_line; return END;

{space}+            // eat up whitespace

.                   return UNK;


%%

