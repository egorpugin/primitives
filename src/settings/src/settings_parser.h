// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

// How to define your own parser?
// 1. Set parser name.
// 2. Choose parser type.
// 3. Include parser helpers          - #include <primitives/helper/bison.h>
// 4. Include parser generated header - #include <THIS_PARSER_NAME.yy.hpp>
// 5. Declare parser.

// 1
#define THIS_PARSER_NAME    settings
#define THIS_PARSER_NAME_UP SETTINGS
#define MY_PARSER           SettingsParser

// 2
#define LALR1_CPP_VARIANT_PARSER

// 3, 4
#include <primitives/helper/bison.h>
#include <settings.yy.hpp>

// 5
DECLARE_PARSER;
