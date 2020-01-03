%{
// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <range_parser.h>

static
#if defined(_WIN32) || defined(__APPLE__)
long
#endif
long make_long(const char *s)
{
    return std::stoll(s);
}

%}

%option nounistd
%option nonoline
%option yylineno
%option nounput
%option batch
%option never-interactive
%option reentrant
%option noyywrap

number [0-9]+
extra  [_a-zA-Z0-9]+
branch [_a-zA-Z][_a-zA-Z0-9]*

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
"=="                    return MAKE(EQ);
"!="                    return MAKE(NE);

"^"                     return MAKE(CARET);
"~"                     return MAKE(TILDE);
"-"                     return MAKE(HYPHEN);
"."                     return MAKE(DOT);

"x"                     |
"X"                     return MAKE_VALUE(X_NUMBER, std::string(yytext));
"*"                     return MAKE(STAR_NUMBER);

"["                     return MAKE(L_SQUARE_BRACKET);
"]"                     return MAKE(R_SQUARE_BRACKET);
"("                     return MAKE(L_BRACKET);
")"                     return MAKE(R_BRACKET);

","                     return MAKE(COMMA);
"&&"                    return MAKE(AND);
"||"                    return MAKE(OR);

{number}                return MAKE_VALUE(NUMBER, make_long(yytext));
{branch}                return MAKE_VALUE(BRANCH, std::string(yytext));
{extra}                 return MAKE_VALUE(EXTRA, std::string(yytext));

<*>(?s:.)               { return MAKE_VALUE(ERROR_SYMBOL, std::string(yytext)); }
<*><<EOF>>              return MAKE(EOQ);

%%
