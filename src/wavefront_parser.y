/* -*- coding: utf-8-unix -*-
 *
 * File: src/wavefront_parser.y
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

int wavefront_lex(WF::ValueType*, WF::LocationType*, yyscan_t);


#define HANDLE_ERROR(item, errnum) {reader->createError(item, errnum); YYERROR;}



%}

%locations
%define parse.trace

%define api.prefix {wavefront_}
%define api.pure full
%define api.value.type {WF::ValueType}

%parse-param {WFReader* reader}
%parse-param {yyscan_t scanner}
%lex-param {yyscan_t scanner}

%token UNK END UNSUPP CSTYPE DEG SURF PARM SURFEND
%token U V_OR_VERTEX TEXCOORD NORMAL FACE LINE_STRIP

%token <v_triplet> VERT
%token <v_int> INT
%token <v_float> FLOAT
%token <v_string> TYPENAME

%type <v_triplets> verts ints
%type <v_ints> controlpoints
%type <v_float> numeric
%type <v_floats> numerics


// Grammar follows


%%

input:
    lines
    ;

lines:
    line
    |
    lines line
    ;

line:
    statement END
    ;

statement:
    unsupported
    |
    supported
    ;

unsupported:
    UNSUPP dummy_argument_list
    ;


dummy_argument_list:
    dummy_argument
    |
    dummy_argument_list dummy_argument
    ;

dummy_argument:
    FLOAT
    |
    INT
    |
    UNK
    ;


supported:
    vertex
    |
    normal
    |
    texcoord
    |
    face
    |
    line_strip
    |
    parametricdef
    |
    poldeg
    |
    surfbegin
    |
    surfparameter
    |
    surfend
    ;


vertex:
    V_OR_VERTEX numeric numeric numeric
        {
            reader->appendVertex($2, $3, $4);
        }
    |
    V_OR_VERTEX numeric numeric numeric numeric
        {
            reader->appendVertex($2, $3, $4, $5);
        }
    ;

numeric:
    FLOAT
        {
            $$ = $1;
        }
    |
    INT
        {
            $$ = $1;
        }
    ;

normal:
    NORMAL numeric numeric numeric
        {
            // unit sphere - normals trivial
        }
    ;

texcoord:
    TEXCOORD numeric
        {
            // unsupported
            // reader->appendTex(0, 0);
        }
    |
    TEXCOORD numeric numeric numeric
        {
            // unsupported
            // reader->appendTex(0, 0);
        }
    |
    TEXCOORD numeric numeric
        {
            // reader->appendTex($2, $3);
        }
    ;

face:
    FACE ints
        {
            reader->appendFace($2);
        }
    |
    FACE verts
        {
            reader->appendFace($2);
        }

    ;

line_strip:
    LINE_STRIP ints
        {
            // reader->appendLineStrip($2);
        }

verts:
    VERT
        {
            $$.clear();
            $$.append(WF::TripletIndex($1[0], $1[1], $1[2]));
        }
    |
    verts VERT
        {
            $$.append(WF::TripletIndex($2[0], $2[1], $2[2]));
        }
    ;

ints:
    INT
        {
            $$.clear();
            $$.append(WF::TripletIndex($1, 0, 0));
        }
    |
    ints INT
        {
            $$.append(WF::TripletIndex($2, 0, 0));
        }
    ;


parametricdef:
    CSTYPE TYPENAME
        {
            // if (reader->inPatchDef()) {
            //     HANDLE_ERROR("cstype", WFReader::Error::InSurfDef);
            // }
            // reader->setPatchType($2);
        }
    ;


surfparameter:
    PARM U numerics
        {
            // if (!reader->inPatchDef()) {
            //     HANDLE_ERROR("parm", WFReader::Error::SurfDefRequired);
            // }
            // reader->setPatchKnots("u", $3);
        }
    |
    PARM V_OR_VERTEX numerics
        {
            // if (!reader->inPatchDef()) {
            //     HANDLE_ERROR("parm", WFReader::Error::SurfDefRequired);
            // }
            // reader->setPatchKnots("v", $3);
        }
    ;

numerics:
    numeric
        {
            $$.clear();
            $$.append($1);
        }
    |
    numerics numeric
        {
            $$.append($2);
        }
    ;

poldeg:
    DEG INT
        {
            // unsupported
        }
    |
    DEG INT INT
        {
            // if (reader->inPatchDef()) {
            //     HANDLE_ERROR("deg", WFReader::Error::InSurfDef);
            // }
            // reader->setPatchRank($2, $3);
        }
    ;

surfbegin:
    SURF numeric numeric numeric numeric controlpoints
        {
            // if (!reader->checkPatchState()) {
            //     HANDLE_ERROR("surf", WFReader::Error::StateNotComplete);
            // }
            // reader->beginPatch($2, $3, $4, $5, $6);
        }
    ;

controlpoints:
    INT
        {
            // $$.clear();
            // $$.append($1);
        }
    |
    controlpoints INT
    {
        // $$.append($2);
    }
    ;

surfend:
    SURFEND
        {
            // reader->endPatch();
        }
    ;


