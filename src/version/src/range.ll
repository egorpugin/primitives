%{
// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <range_parser.h>

// C++, make_ is for variants
#define YY_USER_ACTION loc.columns(yyleng);
%}

%option nounistd
%option yylineno
%option nounput
%option batch
%option never-interactive
%option reentrant
%option noyywrap
%option prefix="ll_range"

number [0-9]+
extra  [_a-zA-Z0-9]+

%%

%{
    // Code run each time yylex is called.
    loc.step();
%}

[ \t\v\f\n\r]+          return MAKE(SPACE);

"<"                     return MAKE(LT);
">"                     return MAKE(GT);
"<="                    return MAKE(LE);
">="                    return MAKE(GE);
"="                     return MAKE(EQ);
"!="                    return MAKE(NE);

"^"                     return MAKE(CARET);
"~"                     return MAKE(TILDE);
"-"                     return MAKE(HYPHEN);
"."                     return MAKE(DOT);

"x"                     |
"X"                     return MAKE_VALUE(X_NUMBER, std::string(yytext));
"*"                     return MAKE(STAR_NUMBER);

","                     return MAKE(AND);
"&&"                    return MAKE(AND);
"||"                    return MAKE(OR);

{number}                return MAKE_VALUE(NUMBER, std::stoll(yytext));
{extra}                 return MAKE_VALUE(EXTRA, std::string(yytext));

.                       { /*printf("err = %s\n", yytext);*/ return MAKE(ERROR_SYMBOL); }
<<EOF>>                 return MAKE(EOQ);

%%
