%{
// Copyright (C) 2020 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <location.h>
#include <cl_parser.h>
%}

%option nounistd
%option nonoline
%option yylineno
%option nounput
%option batch
%option never-interactive
%option reentrant
%option noyywrap

%%

%{
    // Code run each time yylex is called.
    loc.step();
%}

-                       return MAKE(DASH);

[a-zA-Z][a-zA-Z0-9]*    return MAKE_VALUE(STRING, std::string(yytext));
" "                     return MAKE(ARGUMENT_DELIMETER);

<*>(?s:.)               { return MAKE_VALUE(ERROR_SYMBOL, std::string(yytext)); }
<*><<EOF>>              return MAKE(EOQ);

%%
