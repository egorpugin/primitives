// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

%{
#include <settings_parser.h>
%}

////////////////////////////////////////

// general settings
%require "3.0"
//%debug
%start file
%locations
%verbose
//%no-lines
%error-verbose

////////////////////////////////////////

%skeleton "lalr1.cc" // C++ skeleton
%define api.value.type variant
%define api.token.constructor // C++ style of handling variants
%define parse.assert // check C++ variant types

%{
#include <primitives/settings.h>
%}

////////////////////////////////////////

// tokens and types
%token EOQ 0 "end of file"
%token ERROR_SYMBOL

%type <int> file

////////////////////////////////////////

%%

file: EOQ
    {
        $$ = 5;
    }
    ;

%%
