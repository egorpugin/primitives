/*
 * Copyright (C) 2016-2017, Egor Pugin
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// before main include
// Prevent using <unistd.h> because of bug in flex.
#define YY_NO_UNISTD_H 1
#define YY_DECL 1
#include <range.ll.h>

#include "range.h"

#include <sstream>

extern yy_range::parser::symbol_type ll_rangelex(yyscan_t yyscanner, yy_range::location &loc);

yy_range::parser::symbol_type RangeParserDriver::lex()
{
    auto ret = ll_rangelex(scanner, location);
    return ret;
}

void yy_range::parser::error(const location_type& l, const std::string& m)
{
    driver.error(l, m);
}

int RangeParserDriver::parse(const std::string &s)
{
    ll_rangelex_init(&scanner);
    ll_range_scan_string(s.c_str(), scanner);
    auto res = parse();
    ll_rangelex_destroy(scanner);

    return res;
}

int RangeParserDriver::parse()
{
    yy_range::parser parser(*this);
    parser.set_debug_level(debug);
    int res = parser.parse();
    return res;
}

void RangeParserDriver::error(const yy_range::location &l, const std::string &m)
{
    std::ostringstream ss;
    ss << l << " " << m << "\n";
    if (!can_throw)
        std::cerr << ss.str();
    else
        throw std::runtime_error("Error during bazel parse: " + ss.str());
}

void RangeParserDriver::error(const std::string& m)
{
    std::ostringstream ss;
    ss << m << "\n";
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
