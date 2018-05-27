// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <range.yy.hpp>

#include <string>

struct RangeParserDriver
{
    bool debug = true;
    bool can_throw = false;

    int parse(const std::string &s);

    // lex & parse
private:
    void *scanner;
    yy_range::location location;

    int parse();
    yy_range::parser::symbol_type lex();

    void error(const yy_range::location &l, const std::string &m);
    void error(const std::string &m);

    friend yy_range::parser;
};
