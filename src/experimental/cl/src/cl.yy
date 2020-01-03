// Copyright (C) 2020 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

%{
#include <location.h>
#include <cl_parser.h>
%}

////////////////////////////////////////

// general settings
%require "3.0"
%debug
%start command_line
%locations
%verbose
//%no-lines
%define parse.error verbose

////////////////////////////////////////

%glr-parser
%skeleton "glr.cc" // C++ skeleton
%define api.value.type { MY_STORAGE }
%define api.location.type { MyLocation }
%code provides { #include <driver.h> }

//%expect 3

%{
%}

////////////////////////////////////////

// tokens and types
%token EOQ 0 "end of file"
%token ERROR_SYMBOL

// punctuation
%token ARGUMENT_DELIMETER
%token DASH

// simple types
%token <v> STRING

// complex
%type <v> command_line
%type <v> program_name
%type <v> arguments
%type <v> argument

////////////////////////////////////////

%%

command_line: program_name EOQ
    {
// in-rule definition to convert $$
#define SET_RETURN_VALUE(X) SET_VALUE($$, X)
    }
    |
    program_name ARGUMENT_DELIMETER arguments EOQ
    ;

arguments: argument
    | arguments ARGUMENT_DELIMETER argument
    ;

argument: STRING
    | DASH STRING
    {
        EXTRACT_VALUE(String, v, $2);
        if (MY_PARSER_CAST.pd.throw_on_missing_option && !MY_PARSER_CAST.pd.hasOption(v))
        {
            throw std::runtime_error(MY_PARSER_CAST.pd.args[0] + ": Unknown command line argument '-" +
                v + "'. Try: '" + MY_PARSER_CAST.pd.args[0] + " --help'");
        }
        SET_RETURN_VALUE($2);
    }
    ;

program_name: STRING
    ;

%%
