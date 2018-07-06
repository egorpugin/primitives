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
    CHECK(SettingPath("")[0] == "");
    CHECK(SettingPath("a")[0] == "a");
    CHECK(SettingPath(" a")[0] == "a");
    CHECK(SettingPath("a ")[0] == "a");
    CHECK(SettingPath(" a ")[0] == "a");
    CHECK(SettingPath("-")[0] == "-");
    CHECK(SettingPath("_")[0] == "_");
    CHECK(SettingPath("0")[0] == "0");
    CHECK(SettingPath("a-b-c")[0] == "a-b-c");
    CHECK(SettingPath("a_b_c")[0] == "a_b_c");
    CHECK(SettingPath("a-b-c")[0] == "a-b-c");

    {
        auto s = SettingPath("a.b.c");
        CHECK(s[0] == "a");
        CHECK(s[1] == "b");
        CHECK(s[2] == "c");

        s = SettingPath(" a . b . c ");
        CHECK(s[0] == "a");
        CHECK(s[1] == "b");
        CHECK(s[2] == "c");
    }

    // string
    CHECK_THROWS(SettingPath("\""));
    CHECK(SettingPath("\"\"")[0] == "");
    CHECK_THROWS(SettingPath("\"\"\""));
    CHECK(SettingPath("\"\\\" \"")[0] == "\" ");
    CHECK(SettingPath("\"\\\"a \"")[0] == "\"a ");
    CHECK(SettingPath("\"\\\" \"")[0] == "\" ");
    CHECK(SettingPath("\"\\\"a \" . \"\"")[0] == "\"a ");
    CHECK(SettingPath("\"\\\"a \" . \"\"")[1] == "");
    CHECK(SettingPath("\" \\u12aF \" . \" \\U000123aF \"")[0] == u8" \u12Af ");
    CHECK(SettingPath("\" \\u1234 \" . \" \\U000123aF \"")[1] == u8" \U000123Af ");
    CHECK(SettingPath("\" \\u12aF \" . \" \\U000123aF \"").toString() == u8"\" \u12aF \".\" \U000123aF \"");
    CHECK(SettingPath("\"\\n\"")[0] == "\n");
    CHECK(SettingPath("\"\\b\"")[0] == "\b");
    CHECK(SettingPath("\"\\\\n\"")[0] == "\\n");
    CHECK_THROWS(SettingPath("\"\n\""));
    CHECK_THROWS(SettingPath("\"\a\""));
    CHECK_THROWS(SettingPath("\"\\a\""));
    CHECK_THROWS(SettingPath("\"\r\""));
    CHECK_THROWS(SettingPath("\"\r\n\""));
    CHECK_THROWS(SettingPath("\"\"\""));
    CHECK_THROWS(SettingPath("\"\"\"\""));
    CHECK_THROWS(SettingPath("\"\"  \""));
    CHECK_THROWS(SettingPath(" \" \"  \" "));
    CHECK_THROWS(SettingPath(" \" \"\""));

    // literal
    CHECK(SettingPath("''")[0] == "");
    CHECK(SettingPath("'\"\"'")[0] == "\"\"");
    CHECK_THROWS(SettingPath("'"));
    CHECK_THROWS(SettingPath("'''"));
    CHECK_THROWS(SettingPath("''''"));

    // literal copied from basic
    CHECK_THROWS(SettingPath("'"));
    CHECK(SettingPath("''")[0] == "");
    CHECK(SettingPath("'\"'")[0] == "\"");
    CHECK(SettingPath("'\\\"'")[0] == "\\\"");
    CHECK(SettingPath("'\\\" '")[0] == "\\\" ");
    CHECK(SettingPath("'\\\"a '")[0] == "\\\"a ");
    CHECK(SettingPath("'\\\" '")[0] == "\\\" ");
    CHECK(SettingPath("'\\\"a ' . ''")[0] == "\\\"a ");
    CHECK(SettingPath("'\\\"a ' . ''")[1] == "");
    CHECK(SettingPath("' \\u12aF ' . ' \\U000123aF '")[0] == " \\u12aF ");
    CHECK(SettingPath("' \\u1234 ' . ' \\U000123aF '")[1] == " \\U000123aF ");
    CHECK(SettingPath("' \\u12aF ' . ' \\U000123aF '").toString() == u8"\" \\u12aF \".\" \\U000123aF \"");
    CHECK_THROWS(SettingPath("'\a'"));
    CHECK_THROWS(SettingPath("'\b'"));
    CHECK(SettingPath("'\t'")[0] == "\t");
    CHECK(SettingPath("'\\n'")[0] == "\\n");
    CHECK(SettingPath("'\\\\n'")[0] == "\\\\n");
    CHECK_THROWS(SettingPath("'\n'"));
    CHECK_THROWS(SettingPath("'\r'"));
    CHECK_THROWS(SettingPath("'\r\n'"));
    CHECK(SettingPath("'\"'")[0] == "\"");
    CHECK(SettingPath("'\"\"'")[0] == "\"\"");
    CHECK(SettingPath("'\"  '")[0] == "\"  ");
    CHECK(SettingPath(" ' \"  ' ")[0] == " \"  ");
    CHECK(SettingPath(" ' \"'")[0] == " \"");
}

int main(int argc, char **argv)
try
{
    //auto p = primitives::SettingPath("\"\n\"");

    Catch::Session s;
    //while (1)
    s.run(argc, argv);

    using namespace llvm;

    auto &set = primitives::getSettings().getLocalSettings();

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
