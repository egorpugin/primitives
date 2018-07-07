%{
// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <settings_parser.h>
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
newline \r?\n

%x LL_BASIC LL_LITERAL
%x LL_MULTILINE_BASIC LL_MULTILINE_LITERAL

%%

%{
    // Code run each time yylex is called.
    loc.step();
%}

([[:space:]]{-}[\n])+   ;
[[:space:]]+            { return MAKE(NEWLINE); }

"["                     { return MAKE(LEFT_SQUARE_BRACKET); }
"]"                     { return MAKE(RIGHT_SQUARE_BRACKET); }
\=                      { return MAKE(EQUALS); }
\.                      { return MAKE(POINT); }
{bare}                  { return MAKE_VALUE(BARE_STRING, yytext); }

\"                      { BEGIN(LL_BASIC); driver.str.clear(); return MAKE(START_BASIC_STRING); }
<LL_BASIC,LL_MULTILINE_BASIC>[\x00-\xFF]{-}[[:cntrl:]]{-}[\"] { driver.str += *yytext; }
<LL_BASIC,LL_MULTILINE_BASIC>\\[^uU\r\n] {
    auto c = primitives::detail::unescapeSettingChar(*(yytext + 1));
    if (c == 0)
        return MAKE(ERROR_SYMBOL);
    driver.str += c;
}
<LL_BASIC,LL_MULTILINE_BASIC>\\u[[:xdigit:]]{4} {
    int num = 0;
    sscanf(yytext + 2, "%04x", &num);
    driver.str += primitives::detail::cp2utf8(num);
}
<LL_BASIC,LL_MULTILINE_BASIC>\\U[[:xdigit:]]{8} {
    int num = 0;
    sscanf(yytext + 2, "%08x", &num);
    driver.str += primitives::detail::cp2utf8(num);
}
<LL_BASIC>\" {
    BEGIN(0);
    return MAKE_VALUE(BASIC_STRING, driver.str);
}

\"\"\"{newline}?        { BEGIN(LL_MULTILINE_BASIC); driver.str.clear(); return MAKE(START_MULTILINE_BASIC_STRING); }
<LL_MULTILINE_BASIC>{newline} { driver.str += '\n'; }
<LL_MULTILINE_BASIC>\"  { driver.str += *yytext; }
<LL_MULTILINE_BASIC>\\{newline}([[:space:]]*) ;
<LL_MULTILINE_BASIC>\"\"\" {
    BEGIN(0);
    return MAKE_VALUE(MULTILINE_BASIC_STRING, driver.str);
}

\'                      { BEGIN(LL_LITERAL); driver.str.clear(); return MAKE(START_LITERAL_STRING); }
<LL_LITERAL,LL_MULTILINE_LITERAL>([\x00-\xFF]{-}[[:cntrl:]]{-}['])* { driver.str += yytext; }
<LL_LITERAL,LL_MULTILINE_LITERAL>\t { driver.str += *yytext; }
<LL_LITERAL>\'          { BEGIN(0); return MAKE_VALUE(LITERAL_STRING, driver.str); }

\'\'\'{newline}?        { BEGIN(LL_MULTILINE_LITERAL); driver.str.clear(); return MAKE(START_MULTILINE_LITERAL_STRING); }
<LL_MULTILINE_LITERAL>{newline} { driver.str += '\n'; }
<LL_MULTILINE_LITERAL>\' { driver.str += *yytext; }
<LL_MULTILINE_LITERAL>\'\'\' { BEGIN(0); return MAKE_VALUE(MULTILINE_LITERAL_STRING, driver.str); }

<*>(?s:.)               { return MAKE(ERROR_SYMBOL); }
<*><<EOF>>              return MAKE(EOQ);

%%
