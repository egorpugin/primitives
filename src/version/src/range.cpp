// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

// before main include
// Prevent using <unistd.h> because of bug in flex.
#define YY_NO_UNISTD_H 1
#define YY_DECL 1
#include <range.ll.h>

#include "range.h"

#include <iostream>
#include <sstream>

//extern int ll_rangelex(yyscan_t yyscanner, YY_RANGELTYPE &loc);
extern int ll_rangelex(yyscan_t yyscanner, yy_range::parser::location_type &loc);

int RangeParserDriver::lex(YY_RANGESTYPE *val, location_type *loc)
{
    auto ret = ll_rangelex(scanner, *loc);
    return ret;
}

int RangeParserDriver::parse(const std::string &s)
{
    ll_rangelex_init(&scanner);
    ll_range_scan_string(s.c_str(), scanner);
    set_debug_level(debug);
    auto res = base::parse();
    ll_rangelex_destroy(scanner);

    return res;
}

// for some reason this is missing in bison
void yy_range::parser::error(const location_type& loc, const std::string& msg) {}

void RangeParserDriver::error(const location_type& loc, const std::string& msg)
{
    std::ostringstream ss;
    ss << loc << " " << msg << "\n";
    if (!can_throw)
        std::cerr << ss.str();
    else
        throw std::runtime_error("Error during bazel parse: " + ss.str());
}

bool VersionRange::parse(VersionRange &vr, const std::string &s)
{
    RangeParserDriver d;
    return d.parse(s) == 0;
}
