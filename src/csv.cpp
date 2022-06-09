// Copyright (C) 2020 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

//#include <primitives/sw/main.h>
#include <primitives/csv.h>

//#define CATCH_CONFIG_RUNNER
#include <catch2/catch_all.hpp>

#define CHECK_CSV(str, ncols)                       \
    auto cols = parse_line(str, ',', '\"', '\"');   \
    REQUIRE(cols.size() == ncols)

TEST_CASE("Checking csv", "[csv]")
{
    using namespace primitives::csv;

    // 1
    {
        CHECK_CSV("", 1);
        CHECK(!cols[0]);
    }
    {
        CHECK_CSV("a", 1);
        CHECK(cols[0]);
        CHECK(*cols[0] == "a");
    }
    {
        CHECK_CSV("aaaaa", 1);
        CHECK(cols[0]);
        CHECK(*cols[0] == "aaaaa");
    }

    // 2
    {
        CHECK_CSV(",", 2);
        CHECK(!cols[0]);
        CHECK(!cols[1]);
    }
    {
        CHECK_CSV("a,", 2);
        CHECK(cols[0]);
        CHECK(*cols[0] == "a");
        CHECK(!cols[1]);
    }
    {
        CHECK_CSV(",a", 2);
        CHECK(cols[1]);
        CHECK(*cols[1] == "a");
        CHECK(!cols[0]);
    }
    {
        CHECK_CSV("a,a", 2);
        CHECK(cols[0]);
        CHECK(*cols[0] == "a");
        CHECK(cols[1]);
        CHECK(*cols[1] == "a");
    }
    {
        CHECK_CSV("aa,a", 2);
        CHECK(cols[0]);
        CHECK(*cols[0] == "aa");
        CHECK(cols[1]);
        CHECK(*cols[1] == "a");
    }
    {
        CHECK_CSV("a,aa", 2);
        CHECK(cols[0]);
        CHECK(*cols[0] == "a");
        CHECK(cols[1]);
        CHECK(*cols[1] == "aa");
    }
    {
        CHECK_CSV("aa,aa", 2);
        CHECK(cols[0]);
        CHECK(*cols[0] == "aa");
        CHECK(cols[1]);
        CHECK(*cols[1] == "aa");
    }

    // 3
    {
        CHECK_CSV(",,", 3);
        CHECK(!cols[0]);
        CHECK(!cols[1]);
        CHECK(!cols[2]);
    }
    {
        CHECK_CSV("x,x,x", 3);
        CHECK(cols[0]);
        CHECK(*cols[0] == "x");
        CHECK(cols[1]);
        CHECK(*cols[1] == "x");
        CHECK(cols[2]);
        CHECK(*cols[2] == "x");
    }
}

TEST_CASE("Checking csv quoted", "[csv]")
{
    using namespace primitives::csv;

    // 1
    {
        CHECK_CSV(R"xxx("")xxx", 1);
        CHECK(cols[0]);
        CHECK(*cols[0] == "");
    }
    {
        CHECK_CSV(R"xxx("a")xxx", 1);
        CHECK(cols[0]);
        CHECK(*cols[0] == "a");
    }
    {
        CHECK_CSV(R"xxx("aa")xxx", 1);
        CHECK(cols[0]);
        CHECK(*cols[0] == "aa");
    }
    {
        CHECK_CSV(R"xxx("abc")xxx", 1);
        CHECK(cols[0]);
        CHECK(*cols[0] == "abc");
    }

    // 2
    {
        CHECK_CSV(R"xxx("","")xxx", 2);
        CHECK(cols[0]);
        CHECK(cols[1]);
        CHECK(*cols[0] == "");
        CHECK(*cols[1] == "");
    }
    {
        CHECK_CSV(R"xxx("a","")xxx", 2);
        CHECK(cols[0]);
        CHECK(cols[1]);
        CHECK(*cols[0] == "a");
        CHECK(*cols[1] == "");
    }
    {
        CHECK_CSV(R"xxx("","a")xxx", 2);
        CHECK(cols[0]);
        CHECK(cols[1]);
        CHECK(*cols[0] == "");
        CHECK(*cols[1] == "a");
    }
    {
        CHECK_CSV(R"xxx("aa","aa")xxx", 2);
        CHECK(cols[0]);
        CHECK(cols[1]);
        CHECK(*cols[0] == "aa");
        CHECK(*cols[1] == "aa");
    }
}

TEST_CASE("Checking csv quoted with escape symbols", "[csv]")
{
    using namespace primitives::csv;

    // 1
    {
        CHECK_CSV(R"xxx(",")xxx", 1);
        CHECK(cols[0]);
        CHECK(*cols[0] == ",");
    }
    /*{
        CHECK_CSV(R"xxx("",")xxx", 1);
        CHECK(cols[0]);
        CHECK(*cols[0] == ",");
    }*/
    {
        CHECK_CSV(R"xxx("a,a")xxx", 1);
        CHECK(cols[0]);
        CHECK(*cols[0] == "a,a");
    }
    {
        CHECK_CSV(R"xxx("""")xxx", 1);
        CHECK(cols[0]);
        CHECK(*cols[0] == "\"");
    }
    {
        CHECK_CSV(R"xxx("""""")xxx", 1);
        CHECK(cols[0]);
        CHECK(*cols[0] == "\"\"");
    }
    {
        CHECK_CSV(R"xxx(""",""")xxx", 1);
        CHECK(cols[0]);
        CHECK(*cols[0] == "\",\"");
    }
    {
        CHECK_THROWS(parse_line(R"xxx(\")xxx"));
        CHECK_THROWS(parse_line(R"xxx(\)xxx"));
        CHECK_THROWS(parse_line(R"xxx(\")xxx"));
        CHECK_THROWS(parse_line(R"xxx(")xxx", ',', '\"', '\"'));
        CHECK_THROWS(parse_line(R"xxx(""")xxx", ',', '\"', '\"'));
        CHECK_THROWS(parse_line(R"xxx(""""")xxx", ',', '\"', '\"'));
        CHECK_THROWS(parse_line(R"xxx(""""""")xxx", ',', '\"', '\"'));
    }
    {
        /*REQUIRE_NOTHROW(parse_line(R"xxx("\"")xxx"));
        auto cols = parse_line(R"xxx("\"")xxx");
        CHECK(cols[0]);
        CHECK(*cols[0] == "\"");*/
    }
}
