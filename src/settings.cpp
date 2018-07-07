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

TEST_CASE("Checking setting paths", "[setting_path]")
{
    using namespace primitives;

    // bare
    CHECK_THROWS(SettingPath("").at(0) == "");
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

TEST_CASE("Checking settings", "[settings]")
{
    using namespace primitives;

    // basic
    CHECK_NOTHROW(getSettings().getLocalSettings().load("a=b"s));
    CHECK_NOTHROW(getSettings().getLocalSettings().load("a = b"s));
    CHECK_NOTHROW(getSettings().getLocalSettings().load(" a = b "s));
    CHECK_NOTHROW(getSettings().getLocalSettings().load("a = \"\"\"b\"\"\""s));
    CHECK_NOTHROW(getSettings().getLocalSettings().load("a = \"\"\" b \n \"\"\""s));
    CHECK_NOTHROW(getSettings().getLocalSettings().load("a = \"\"\" b \r\n \"\"\""s));
    CHECK_NOTHROW(getSettings().getLocalSettings().load("a = \"\"\" b \" \r\n \"\"\""s));
    CHECK_NOTHROW(getSettings().getLocalSettings().load("a = \"\"\" b \"\" \r\n \"\"\""s));
    CHECK_NOTHROW(getSettings().getLocalSettings().load("a = \"\"\" b \"\" ''' \r\n \"\"\""s));
    CHECK_NOTHROW(getSettings().getLocalSettings().load("a = \"\"\" b \"\" ' '' \r\n \"\"\""s));
    CHECK_NOTHROW(getSettings().getLocalSettings().load("\r\n\v\t\f  a = \"\"\" b \"\" ' '' \r\n \"\"\"  "s));
    CHECK_NOTHROW(getSettings().getLocalSettings().load("\r\n\v\t\f  a = \"\"\" b \"\" ' '' \r\n \"\"\"  \r\n\v\t\f  "s));
    CHECK_THROWS(getSettings().getLocalSettings().load("a = \"\"\" b \"\"\" \r\n \"\"\""s));
    CHECK_THROWS(getSettings().getLocalSettings().load("a = \"\"\" b \r \"\"\""s));
    CHECK_THROWS(getSettings().getLocalSettings().load("a = \"\"\"\rb\"\"\""s));
    {
        CHECK_NOTHROW(getSettings().getLocalSettings().load("a = \"\"\"\r\nb\"\"\""s));
        CHECK(getSettings().getLocalSettings()["a"] == "b");
    }
    {
        CHECK_NOTHROW(getSettings().getLocalSettings().load("a = \"\"\"\nb\"\"\""s));
        CHECK(getSettings().getLocalSettings()["a"] == "b");
    }

    // line ending backslash: only basic
    {
        String cfg;

        cfg =
            "a = \"\"\""
            "bc"
            "\"\"\""
            ;
        CHECK_NOTHROW(getSettings().getLocalSettings().load(cfg));
        CHECK(getSettings().getLocalSettings()["a"] == "bc");

        cfg =
            "a = \"\"\" "
            "bc"
            "\"\"\""
            ;
        CHECK_NOTHROW(getSettings().getLocalSettings().load(cfg));
        CHECK(getSettings().getLocalSettings()["a"] == " bc");

        cfg =
            "a = \"\"\" \n"
            "bc"
            "\"\"\""
            ;
        CHECK_NOTHROW(getSettings().getLocalSettings().load(cfg));
        CHECK(getSettings().getLocalSettings()["a"] == " \nbc");

        cfg =
            "a = \"\"\" \r\n"
            "bc"
            "\"\"\""
            ;
        CHECK_NOTHROW(getSettings().getLocalSettings().load(cfg));
        CHECK(getSettings().getLocalSettings()["a"] == " \nbc");

        cfg =
            "a = \"\"\"\n"
            "bc"
            "\"\"\""
            ;
        CHECK_NOTHROW(getSettings().getLocalSettings().load(cfg));
        CHECK(getSettings().getLocalSettings()["a"] == "bc");

        cfg =
            "a = \"\"\"\r\n"
            "bc"
            "\"\"\""
            ;
        CHECK_NOTHROW(getSettings().getLocalSettings().load(cfg));
        CHECK(getSettings().getLocalSettings()["a"] == "bc");

        cfg = R"(
            a = """
bc
""")";
        CHECK_NOTHROW(getSettings().getLocalSettings().load(cfg));
        CHECK(getSettings().getLocalSettings()["a"] == "bc\n");

        cfg = R"(
            a = """
bc\
""")";
        CHECK_NOTHROW(getSettings().getLocalSettings().load(cfg));
        CHECK(getSettings().getLocalSettings()["a"] == "bc");

        cfg = R"(
            a = """ \
bc\
""")";
        CHECK_NOTHROW(getSettings().getLocalSettings().load(cfg));
        CHECK(getSettings().getLocalSettings()["a"] == " bc");

        cfg = R"(
            a = """
 \
bc\
""")";
        CHECK_NOTHROW(getSettings().getLocalSettings().load(cfg));
        CHECK(getSettings().getLocalSettings()["a"] == " bc");

        cfg = R"(
            a = """ \


                    bc\
""")";
        CHECK_NOTHROW(getSettings().getLocalSettings().load(cfg));
        CHECK(getSettings().getLocalSettings()["a"] == " bc");

        cfg = R"(
            a = """\


                    bc\
""")";
        CHECK_NOTHROW(getSettings().getLocalSettings().load(cfg));
        CHECK(getSettings().getLocalSettings()["a"] == "bc");

        cfg = R"(
            a = """\


                    bc\


                d\
""")";
        CHECK_NOTHROW(getSettings().getLocalSettings().load(cfg));
        CHECK(getSettings().getLocalSettings()["a"] == "bcd");

        cfg = R"(
            a = """\


                    bc \


                d\
""")";
        CHECK_NOTHROW(getSettings().getLocalSettings().load(cfg));
        CHECK(getSettings().getLocalSettings()["a"] == "bc d");

        cfg = R"(
            a = """ \


                    bc \


                d \
""")";
        CHECK_NOTHROW(getSettings().getLocalSettings().load(cfg));
        CHECK(getSettings().getLocalSettings()["a"] == " bc d ");
    }

    // literal copied from basic
    CHECK_NOTHROW(getSettings().getLocalSettings().load("a=b"s));
    CHECK_NOTHROW(getSettings().getLocalSettings().load("a = b"s));
    CHECK_NOTHROW(getSettings().getLocalSettings().load(" a = b "s));
    CHECK_NOTHROW(getSettings().getLocalSettings().load("a = '''b'''"s));
    CHECK_NOTHROW(getSettings().getLocalSettings().load("a = ''' b \n '''"s));
    CHECK_NOTHROW(getSettings().getLocalSettings().load("a = ''' b \r\n '''"s));
    CHECK_NOTHROW(getSettings().getLocalSettings().load("a = ''' b ' \r\n '''"s));
    CHECK_NOTHROW(getSettings().getLocalSettings().load("a = ''' b '' \r\n '''"s));
    CHECK_NOTHROW(getSettings().getLocalSettings().load("a = ''' b \"\" \"\"\" \r\n '''"s));
    CHECK_NOTHROW(getSettings().getLocalSettings().load("\r\n\v\t\f  a = ''' b \"\" \" \"\" \r\n '''  "s));
    CHECK_NOTHROW(getSettings().getLocalSettings().load("\r\n\v\t\f  a = ''' b \"\" \" \"\" \r\n '''  \r\n\v\t\f  "s));
    CHECK_THROWS(getSettings().getLocalSettings().load("a = ''' b ''' \r\n '''"s));
    CHECK_THROWS(getSettings().getLocalSettings().load("a = ''' b \r '''"s));
    CHECK_THROWS(getSettings().getLocalSettings().load("a = '''\rb'''"s));
    {
        CHECK_NOTHROW(getSettings().getLocalSettings().load("a = '''\r\nb'''"s));
        CHECK(getSettings().getLocalSettings()["a"] == "b");
    }
    {
        CHECK_NOTHROW(getSettings().getLocalSettings().load("a = '''\nb'''"s));
        CHECK(getSettings().getLocalSettings()["a"] == "b");
    }

    {
        String cfg =
            "\r\n\v\t\f  e.b.c = \"\"\" b \"\" ' '' \r\n \"\"\"  \r\n\v\t\f  "
            "a=b\n"
            "b=a\r\n"
            "b.c.d=a\r\n"
            "b . c . d . e=abc\r\n"
            "\r\n\v\t\f  a.b.c.d = ''' b \"\" \" \"\" \r\n '''  \r\n\v\t\f  "
        ;
        CHECK_NOTHROW(getSettings().getLocalSettings().load(cfg));
        CHECK(getSettings().getLocalSettings()["a"] == "b");
        CHECK(getSettings().getLocalSettings()["b"] == "a");
        CHECK(getSettings().getLocalSettings()["b.c.d"] == "a");
        CHECK(getSettings().getLocalSettings()["b.c.d.e"] == "abc");
    }

    // table
    {
        CHECK_THROWS(getSettings().getLocalSettings().load("[a]b=c"s));
        CHECK_NOTHROW(getSettings().getLocalSettings().load("[a]\nb=c"s));
        CHECK_NOTHROW(getSettings().getLocalSettings().load("[a]"s));

        String cfg =
            "[a]\n"
            "b=c\n"
            ;
        CHECK_NOTHROW(getSettings().getLocalSettings().load(cfg));
        CHECK(getSettings().getLocalSettings()["a.b"] == "c");
    }

    {
        String cfg = R"settings_config(

)settings_config";
        CHECK_NOTHROW(getSettings().getLocalSettings().load(cfg));
    }
}

primitives::SettingStorage<primitives::Settings> &getSettings()
{
    static primitives::SettingStorage<primitives::Settings> &settings = []() -> decltype(auto)
    {
        auto &s = primitives::getSettings();
        //s.configFilename = "";
        return s;
    }();
    return settings;
}

int main(int argc, char **argv)
try
{
    Catch::Session s;
    //while (1)
    s.run(argc, argv);

    using namespace llvm;


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
