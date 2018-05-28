// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "range.h"

#include <iostream>
#include <sstream>

// forward decls
int LEXER_NAME(lex_init)(void **scanner);
int LEXER_NAME(lex_destroy)(void *yyscanner);
struct yy_buffer_state *LEXER_NAME(_scan_string)(const char *yy_str, void *yyscanner);

int MY_PARSER::lex(PARSER_NAME_UP(STYPE) *val, location_type *loc)
{
    auto ret = LEXER_NAME(lex)(scanner, *loc);
    if (std::get<1>(ret).has_value())
        val->setValue(*this, std::get<1>(ret));
    return std::get<0>(ret);
}

int MY_PARSER::parse(const std::string &s)
{
    LEXER_NAME(lex_init)(&scanner);
    LEXER_NAME(_scan_string)(s.c_str(), scanner);
    set_debug_level(debug);
    auto res = base::parse();
    LEXER_NAME(lex_destroy)(scanner);

    return res;
}

void MY_PARSER::error(const location_type& loc, const std::string& msg)
{
    std::ostringstream ss;
    ss << loc << " " << msg << "\n";
    if (!can_throw)
        std::cerr << ss.str();
    else
        throw std::runtime_error("Error during bazel parse: " + ss.str());
}

// for some reason this is missing in bison
void THIS_PARSER::parser::error(const location_type& loc, const std::string& msg) {}

// user of this parser
#include <primitives/version.h>

namespace primitives::version
{

bool VersionRange::parse(VersionRange &vr, const std::string &s)
{
    MY_PARSER d;
    auto r = d.parse(s);
    vr = d.getResult<VersionRange>();
    return r == 0;
}

}
