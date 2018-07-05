// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

// How to define your own parser?
// 1. Write it names - setup parser
// 2. Write lex macro.
// 3. Choose parser type.
// 4. Include parser helpers          - #include <primitives/helper/bison.h>
// 5. Include parser generated header - #include <THIS_PARSER_NAME.yy.hpp>
// 6. Declare parser.

// 1
// setup parser
#define THIS_PARSER_NAME settings
#define THIS_PARSER_NAME_UP SETTINGS
#define MY_PARSER SettingsParser

// 2
//      vvvvvvvv change here
#define yy_settingslex(val,loc) MY_PARSER_CAST.lex(val,loc)

// 3
#define LALR1_CPP_VARIANT_PARSER

// 4, 5
#include <primitives/helper/bison.h>
#include <settings.yy.hpp>

// 6
DECLARE_PARSER;
