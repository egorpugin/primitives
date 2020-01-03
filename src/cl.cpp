// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#define NOMINMAX
#include <primitives/cl.h>
#include <primitives/filesystem.h>
#include <primitives/sw/main.h>

#include <boost/algorithm/string.hpp>

#include <chrono>
#include <iostream>

#define CATCH_CONFIG_RUNNER
#include <catch2/catch.hpp>

#define REQUIRE_TIME(e, t)                                                                       \
    do                                                                                           \
    {                                                                                            \
        auto t1 = get_time([&] { CHECK(e); });                                                   \
        if (t1 > t)                                                                              \
        {                                                                                        \
            auto d = std::chrono::duration_cast<std::chrono::duration<float>>(t1 - t).count();   \
            REQUIRE_NOTHROW(throw std::runtime_error("time exceed on " #e + std::to_string(d))); \
        }                                                                                        \
    } while (0)

#define REQUIRE_NOTHROW_TIME(e, t)  \
    if (get_time([&] { (e); }) > t) \
    REQUIRE_NOTHROW(throw std::runtime_error("time exceed on " #e))

#define REQUIRE_NOTHROW_TIME_MORE(e, t) \
    if (get_time([&] { (e); }) <= t)    \
    REQUIRE_NOTHROW(throw std::runtime_error("time exceed on " #e))

TEST_CASE("Checking cl", "[cl]")
{
    using namespace primitives::cl;

    auto parse_no_ex = [](const Strings &args)
    {
        ProgramDescription pd;
        pd.throw_on_missing_option = false;
        pd.args = args;
        parse(pd);
    };

    CHECK_THROWS(parse_no_ex({})); // no args
    CHECK_THROWS(parse_no_ex({""}));      // empty prog
    CHECK_NOTHROW(parse_no_ex({"sw"}));   // prog

    // simple dash
    CHECK_NOTHROW(parse_no_ex({"sw", "-a"}));
    CHECK_NOTHROW(parse_no_ex({"sw", "-a", "-b"}));
    CHECK_NOTHROW(parse_no_ex({"sw", "-a", "-b", "-c"}));
    CHECK_NOTHROW(parse_no_ex({"sw", "-a", "-b", "-c", "-d"}));
    CHECK_NOTHROW(parse_no_ex({"sw", "-a", "-b", "-c", "-d", "-e"}));

    // simple word
    CHECK_NOTHROW(parse_no_ex({"sw", "a"}));
    CHECK_NOTHROW(parse_no_ex({"sw", "a", "b"}));
    CHECK_NOTHROW(parse_no_ex({"sw", "a", "b", "c"}));
    CHECK_NOTHROW(parse_no_ex({"sw", "a", "b", "c", "d"}));
    CHECK_NOTHROW(parse_no_ex({"sw", "a", "b", "c", "d", "e"}));

    // mixed
    CHECK_NOTHROW(parse_no_ex({"sw", "-a", "b"}));
    CHECK_NOTHROW(parse_no_ex({"sw", "a", "-b"}));
    CHECK_NOTHROW(parse_no_ex({"sw", "-a", "b", "-c"}));
    CHECK_NOTHROW(parse_no_ex({"sw", "-a", "b", "-c", "d"}));
    CHECK_NOTHROW(parse_no_ex({"sw", "-a", "b", "-c", "d", "-e"}));
}

int main(int argc, char **argv)
{
    auto r = Catch::Session().run(argc, argv);
    return r;
}
