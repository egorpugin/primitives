// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <primitives/settings.h>

#include <llvm/Support/CommandLine.h>

#include <chrono>
#include <iostream>

#define CATCH_CONFIG_RUNNER
#include <catch.hpp>

TEST_CASE("Checking settings", "[settings]")
{
    using namespace primitives;

    // bare
    CHECK(parseSettingPath("")[0] == "");
    CHECK(parseSettingPath("a")[0] == "a");
    CHECK(parseSettingPath(" a")[0] == "a");
    CHECK(parseSettingPath("a ")[0] == "a");
    CHECK(parseSettingPath(" a ")[0] == "a");
    CHECK(parseSettingPath("-")[0] == "-");
    CHECK(parseSettingPath("_")[0] == "_");
    CHECK(parseSettingPath("0")[0] == "0");
    CHECK(parseSettingPath("a-b-c")[0] == "a-b-c");
    CHECK(parseSettingPath("a_b_c")[0] == "a_b_c");
    CHECK(parseSettingPath("a-b-c")[0] == "a-b-c");

    {
        auto s = parseSettingPath("a.b.c");
        CHECK(s[0] == "a");
        CHECK(s[1] == "b");
        CHECK(s[2] == "c");

        s = parseSettingPath(" a . b . c ");
        CHECK(s[0] == "a");
        CHECK(s[1] == "b");
        CHECK(s[2] == "c");
    }

    // string
    CHECK(parseSettingPath("\"\"")[0] == "");
    CHECK(parseSettingPath("\"\\\" \"")[0] == "\"");
    CHECK(parseSettingPath("\"\\\"a \"")[0] == "\"a ");
    CHECK(parseSettingPath("\"\\\" \"")[0] == "\" ");
    CHECK(parseSettingPath("\"\\\"a \" . \"\"")[0] == "\"a ");
    CHECK(parseSettingPath("\"\\\"a \" . \"\"")[1] == "");
    CHECK(parseSettingPath("\" \u1234 \" . \" \U00002345 \"")[0] == "\"");
    CHECK(parseSettingPath("\"\\n\"")[0] == "\n");
    CHECK_THROWS(parseSettingPath("\"\n\""));
    CHECK_THROWS(parseSettingPath("\"\"\""));
    CHECK_THROWS(parseSettingPath("\"\"\"\""));
    CHECK_THROWS(parseSettingPath("\"\"  \""));
    CHECK_THROWS(parseSettingPath(" \" \"  \" "));
    CHECK_THROWS(parseSettingPath(" \" \"\""));

    // literal
    CHECK(parseSettingPath("''")[0] == "");
    CHECK(parseSettingPath("'\"\"'")[0] == "\"\"");
    CHECK_THROWS(parseSettingPath("'''"));
    CHECK_THROWS(parseSettingPath("''''"));
}

int main(int argc, char **argv)
try
{
    Catch::Session s;
    s.run(argc, argv);

    using namespace llvm;

    auto &set = getSettings().getLocalSettings();

    cl::opt<std::string> OutputFilename("o", cl::desc("Specify output filename"), cl::value_desc("filename"));

    cl::ParseCommandLineOptions(argc, argv);

    std::ofstream Output(OutputFilename.c_str());

    return 0;
}
catch (std::exception &e)
{
    std::cerr << e.what() << "\n";
    return 1;
}
catch (...)
{
    std::cerr << "unknown exception" << "\n";
    return 1;
}
