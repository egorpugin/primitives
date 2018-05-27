// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

%{
#include <range.h>

// C++
#define YY_RANGESTYPE int

#define yy_rangelex(val,loc) ((RangeParserDriver*)&yyparser)->lex(val,loc)
// C
//#define yy_rangeerror(p,e) p.error(e)
%}

////////////////////////////////////////

// general settings
%require "3.0"
%debug
%start file
%locations
%verbose
%no-lines
%error-verbose

////////////////////////////////////////

%glr-parser

// c++ skeleton and options
%skeleton "glr.cc"

%define api.prefix {yy_range}
//%define api.value.type union
//%define api.value.type {struct my_data}
//%define api.value.type variant
//%define api.token.constructor // C++ style of handling variants
//%define parse.assert // check C++ variant types

%code requires // forward decl of C++ driver (our parser) in HPP
{
#pragma warning(disable: 4005)
#pragma warning(disable: 4065)

// C
//#define YY_RANGESTYPE void*

// C++
#define YY_RANGESTYPE int

#include <primitives/version.h>
using namespace primitives::version;

}

////////////////////////////////////////

// tokens and types
%token EOQ 0 "end of file"
%token ERROR_SYMBOL

%token SPACE
%token LT GT LE GE EQ NE
%token AND
%token OR
%token XNUMBER CARET TILDE HYPHEN DOT

%token <std::string> EXTRA
%token <Version::Number> NUMBER

%type <VersionRange> range_set_or

////////////////////////////////////////

%%

file: range_set_or EOQ
    ;

range_set_or: range_set_and
    | range_set_or or range_set_and
    ;

range_set_and: range
    | range_set_and and range
    ;

range: compare | caret | tilde | hyphen | version
    ;

compare:
      LT space_or_empty version
    | GT space_or_empty version
    | LE space_or_empty version
    | GE space_or_empty version
    | EQ space_or_empty version
    | NE space_or_empty version
    ;

caret: CARET space_or_empty version
    ;

tilde: TILDE space_or_empty version
    ;

hyphen: version SPACE HYPHEN SPACE version
    ;

version: basic_version
    | basic_version HYPHEN extra
    ;

basic_version:
      number
    | number DOT number
    | number DOT number DOT number
    | number DOT number DOT number DOT number
    ;

number: NUMBER | XNUMBER
    ;

extra: EXTRA
    | extra DOT EXTRA
    ;

or: space_or_empty OR space_or_empty
    ;

and: SPACE
    | space_or_empty AND space_or_empty
    ;

space_or_empty: /* empty */
    | SPACE
    ;

%%
