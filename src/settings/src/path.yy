// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

%{
#include <path_parser.h>
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
%code provides { #include <primitives/helper/bison_yy.h> }
%parse-param { MY_PARSER_DRIVER &driver } // param to yy::parser() constructor (the parsing context)

// prevent duplication of locations
%define api.location.type { yy_settings::location }
%code requires { #include <location.hh> }

%code requires
{
#include <primitives/settings.h>
}

%code provides
{
struct MY_PARSER_DRIVER : MY_PARSER
{
    primitives::SettingPath::Parts sp;
    std::string str;
};
}

////////////////////////////////////////

// tokens and types
%token EOQ 0 "end of file"
%token ERROR_SYMBOL LL_FATAL_ERROR

%token POINT

%token <std::string> BARE_STRING BASIC_STRING LITERAL_STRING
%token START_BASIC_STRING START_LITERAL_STRING

%type <std::string> part
%type <primitives::SettingPath::Parts> file path parts

////////////////////////////////////////

%%

file: path EOQ
    {
        driver.sp = $1;
    }
    ;

path: /* empty */
    {}
    | parts
    { $$ = $1; }
    ;

parts: part
    { $$.push_back($1); }
    | parts POINT part
    {
        $1.push_back($3);
        $$ = $1;
    }
    ;

part: BARE_STRING
    { $$ = $1; }
    | START_BASIC_STRING BASIC_STRING
    { $$ = $2; }
    | START_LITERAL_STRING LITERAL_STRING
    { $$ = $2; }
    ;

%%
