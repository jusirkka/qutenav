%{

#include "s52presentation_p.h"
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
unum           {dig}+

singlechar     [;,]

/* ascii >= 64 says the spec */
colorname      [@A-Z\[\\\]\^_\`a-z\{\|\}~]

%%

{unum}              yylval->v_int = static_cast<int>(strtol(yytext, NULL, 0)); return INT;

"SP"                return SP;
"ST"                return ST;
"SW"                return SW;
"PU"                return PU;
"PD"                return PD;
"CI"                return CI;
"PM"                return PM;
"EP"                return EP;
"FP"                return FP;

{colorname}         yylval->v_char = *yytext; return COLOR;
{singlechar}        yylval->v_char = *yytext; return *yytext;



%%

