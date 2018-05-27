// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <range.yy.hpp>

#include <string>

struct RangeParserDriver : yy_range::parser
{
    using base = yy_range::parser;

    bool debug = false;
    bool can_throw = false;

    int parse(const std::string &s);

    // C
    //int lex();
    //void error(const std::string &m);

    // C++
    int lex(YY_RANGESTYPE *val, location_type *loc);

    // lex & parse
private:
    void *scanner;

    // C
    //YY_RANGELTYPE location;

    // C++
    void error(const location_type &loc, const std::string& msg) override;
};
