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
%code provides { #include <primitives/helper/bison_yy.h> }
%parse-param { MY_PARSER_DRIVER &driver } // param to yy::parser() constructor (the parsing context)

%code requires
{
#include <primitives/settings.h>
}

%code provides
{
struct MY_PARSER_DRIVER : MY_PARSER
{
    std::string str;
    primitives::SettingPath key;
    primitives::Settings::SettingsMap sm;
};
}

////////////////////////////////////////

// tokens and types
%token EOQ 0 "end of file"
%token ERROR_SYMBOL

%token POINT EQUALS NEWLINE
%token LEFT_SQUARE_BRACKET RIGHT_SQUARE_BRACKET

%token <std::string> BARE_STRING BASIC_STRING LITERAL_STRING
%token <std::string> MULTILINE_BASIC_STRING MULTILINE_LITERAL_STRING
%token START_BASIC_STRING START_LITERAL_STRING
%token START_MULTILINE_BASIC_STRING START_MULTILINE_LITERAL_STRING

%type <std::string> part
%type <primitives::Settings> file
%type <primitives::Settings::SettingsMap> settings
%type <std::pair<primitives::SettingPath, primitives::Setting>> setting
%type <primitives::SettingPath> key
%type <std::string> value multiline_part
%type <primitives::SettingPath::Parts> path parts

////////////////////////////////////////

%%

file: newline_optional
    {}
    | newline_optional settings newline_optional EOQ
    {
        driver.sm = $2;
    }
    ;

settings: setting
    {
        $$.insert($1);
    }
    | settings newline setting
    {
        $1.insert($3);
        $$ = $1;
    }
    | table
    {}
    ;

table: LEFT_SQUARE_BRACKET key RIGHT_SQUARE_BRACKET
    { driver.key = $2; }
    ;

setting: key EQUALS value
    {
        $$.first = driver.key / $1;
        $$.second = $3;
    }
    ;

key: path
    { $$.parts = $1; }
    ;

value: multiline_part
    { $$ = $1; }
    ;

multiline_part: part
    { $$ = $1; }
    | START_MULTILINE_BASIC_STRING MULTILINE_BASIC_STRING
    { $$ = $2; }
    | START_MULTILINE_LITERAL_STRING MULTILINE_LITERAL_STRING
    { $$ = $2; }
    ;

path: parts
    { $$ = $1; }
    ;

parts: part
    {
        primitives::SettingPath::Parts sp;
        sp.push_back($1);
        $$ = sp;
    }
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

newline: NEWLINE
    ;

newline_optional: /* empty */
    | newline
    ;

%%
