// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

%{
#include <settings_parser.h>

#include <limits.h> // for int max/min
%}

////////////////////////////////////////

// general settings
%require "3.0"
//%debug
%start file
%locations
%verbose
//%no-lines
%define parse.error verbose

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
    primitives::SettingPath table_key;
    primitives::Settings::SettingsMap sm;
};
}

////////////////////////////////////////

// tokens and types
%token EOQ 0 "end of file"
%token ERROR_SYMBOL

%token POINT COMMA KV_DELIMETER
%token LEFT_SQUARE_BRACKET RIGHT_SQUARE_BRACKET LEFT_CURLY_BRACKET RIGHT_CURLY_BRACKET
%token <bool> VALUE_BOOL
%token <double> VALUE_DOUBLE
%token <int64_t> VALUE_INTEGER

%token <std::string> BARE_STRING BASIC_STRING LITERAL_STRING BARE_RHS_STRING
%token <std::string> MULTILINE_BASIC_STRING MULTILINE_LITERAL_STRING
%token START_BASIC_STRING START_LITERAL_STRING
%token START_MULTILINE_BASIC_STRING START_MULTILINE_LITERAL_STRING

// TODO:
// WE MUST NOT PARSE int, float types here!
// only check them and return as string.
// caller will choose expected type and convert to it!!!

%type <bool> bool
%type <int64_t> integer
%type <double> float
%type <std::vector<primitives::Setting>> array values values1
%type <std::string> part_string bare_string line_string multiline_string
%type <primitives::Settings> file
%type <primitives::Settings::SettingsMap> setting settings inline_settings inline_settings1 inline_table
%type <primitives::SettingPath> key
%type <primitives::Setting> value
%type <primitives::SettingPath::Parts> path parts

////////////////////////////////////////

%%

file: /* empty */
    {}
    | settings EOQ
    {
        driver.sm = $1;
    }
    ;

settings: setting
    {
        $$.insert($1.begin(), $1.end());
    }
    | settings setting
    {
        $1.insert($2.begin(), $2.end());
        $$ = $1;
    }
    ;

setting: key KV_DELIMETER value
    {
        $$.emplace(driver.table_key / $1, $3);
    }
    | key KV_DELIMETER inline_table
    {
        for (auto &[k, v] : $3)
            $$.emplace(driver.table_key / $1 / k, v);
    }
    | table
    {}
    ;

table: LEFT_SQUARE_BRACKET key RIGHT_SQUARE_BRACKET
    { driver.table_key = $2; }
    ;

inline_table: LEFT_CURLY_BRACKET inline_settings1 RIGHT_CURLY_BRACKET
    { $$ = $2; }
    ;

inline_settings1: /* empty */
    {}
    | inline_settings comma_optional
    { $$ = $1; }
    ;

inline_settings: setting
    {
        $$.insert($1.begin(), $1.end());
    }
    | inline_settings comma setting
    {
        $1.insert($3.begin(), $3.end());
        $$ = $1;
    }
    ;

key: path
    { $$.parts = $1; }
    ;

value: multiline_string
    { $$ = $1; }
    | bool
    { $$ = $1; }
    | integer
    {
        if ($1 > INT_MAX || $1 < INT_MIN)
            $$ = $1;
        else
            $$ = (int)$1;
    }
    | float
    { $$ = $1; }
    | array
    {
        // TODO: implement
        // $$ = $1;
    }
    ;

array: LEFT_SQUARE_BRACKET values1 RIGHT_SQUARE_BRACKET
    { $$ = $2; }
    ;

values1: /* empty */
    {}
    | values comma_optional
    { $$ = $1; }
    ;

values: value
    {
        $$.push_back($1);
    }
    | values comma value
    {
        $1.push_back($3);
        $$ = $1;
    }
    ;

path: parts
    { $$ = $1; }
    ;

parts: part_string
    {
        primitives::SettingPath::Parts sp;
        sp.push_back($1);
        $$ = sp;
    }
    | parts POINT part_string
    {
        $1.push_back($3);
        $$ = $1;
    }
    ;

part_string: bare_string
    { $$ = $1; }
    | line_string
    { $$ = $1; }
    ;

integer: VALUE_INTEGER
    { $$ = $1; }
    ;

float: VALUE_DOUBLE
    { $$ = $1; }
    ;

bool: VALUE_BOOL
    { $$ = $1; }
    ;

multiline_string: line_string
    { $$ = $1; }
    | START_MULTILINE_BASIC_STRING MULTILINE_BASIC_STRING
    { $$ = $2; }
    | START_MULTILINE_LITERAL_STRING MULTILINE_LITERAL_STRING
    { $$ = $2; }
    | BARE_RHS_STRING
    { $$ = $1; }
    ;

line_string: START_BASIC_STRING BASIC_STRING
    { $$ = $2; }
    | START_LITERAL_STRING LITERAL_STRING
    { $$ = $2; }
    ;

bare_string: BARE_STRING
    { $$ = $1; }
    ;

comma: COMMA
    ;

comma_optional: /* empty */
    | comma
    ;

%%
