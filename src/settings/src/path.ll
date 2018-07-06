%{
// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <path_parser.h>
%}

%option nounistd
%option nonoline
%option yylineno
%option nounput
%option batch
%option never-interactive
%option reentrant
%option noyywrap

bare [a-zA-Z0-9_-]*

%x LL_BASIC LL_LITERAL

%%

%{
    // Code run each time yylex is called.
    loc.step();
%}

[[:space:]]             ;

"."                     { return MAKE(POINT); }
{bare}                  { return MAKE_VALUE(BARE_STRING, yytext); }

\"                      { BEGIN(LL_BASIC); driver.str.clear(); return MAKE(START_BASIC_STRING); }
<LL_BASIC>[\x00-\xFF]{-}[[:cntrl:]]{-}[\"] { driver.str += *yytext; }
<LL_BASIC>\\[^uU] {
    auto c = *(yytext + 1);
    switch (c)
    {
#define M(f, t)          \
    case f:              \
        driver.str += t; \
        break

    M('n', '\n');
    M('\"', '\"');
    M('\\', '\\');
    M('r', '\r');
    M('t', '\t');
    M('f', '\f');
    M('b', '\b');

    default:
        return MAKE(ERROR_SYMBOL);
    }
}
<LL_BASIC>\\u[[:xdigit:]]{4} {
    int num = 0;
    sscanf(yytext + 2, "%04x", &num);
    driver.str += primitives::detail::cp2utf8(num);
}
<LL_BASIC>\\U[[:xdigit:]]{8} {
    int num = 0;
    sscanf(yytext + 2, "%08x", &num);
    driver.str += primitives::detail::cp2utf8(num);
}
<LL_BASIC>\" {
    BEGIN(0);
    return MAKE_VALUE(BASIC_STRING, driver.str);
}

\'                      { BEGIN(LL_LITERAL); driver.str.clear(); return MAKE(START_BASIC_STRING); }
<LL_LITERAL>([\x00-\xFF]{-}[[:cntrl:]]{-}['])* { driver.str += yytext; }
<LL_LITERAL>\t          { driver.str += *yytext; }
<LL_LITERAL>\'          { BEGIN(0); return MAKE_VALUE(BASIC_STRING, driver.str); }

<*>(?s:.)               { return MAKE(ERROR_SYMBOL); }
<*><<EOF>>              return MAKE(EOQ);

%%
