%{
// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <path_parser.h>
%}

%option nounistd
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

.                       { /*printf("err = %s\n", yytext);*/ return MAKE(ERROR_SYMBOL); }
<<EOF>>                 return MAKE(EOQ);

%%
