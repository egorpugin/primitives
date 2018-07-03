// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

// How to define your own parser?
// 1. Write it names - setup parser
// 2. Write lex macro.
// 3. Include parser helpers          - #include <parser_common.h>
// 4. Include parser generated header - #include <range.yy.hpp>
// 5. Write flex declarations.
// 6. Declare parser.

// 1
// setup parser
#define THIS_PARSER_NAME range
#define THIS_PARSER_NAME_UP RANGE
#define MY_PARSER RangeParser

// 2
//      vvvvvvvv change here
#define yy_rangelex(val,loc) MY_PARSER_CAST.lex(val,loc)

// 3, 4
#include <primitives/helper/bison.h>
#include <range.yy.hpp>

// 5
DECLARE_PARSER;
