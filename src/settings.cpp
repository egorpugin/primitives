// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <primitives/main.h>
#include <primitives/settings.h>
#include <primitives/sw/settings.h>

#include <chrono>
#include <iostream>

#define CATCH_CONFIG_RUNNER
#include <catch.hpp>

int argc;
char **argv;

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

TEST_CASE("Checking setting", "[setting]")
{
    // setting
    {
        primitives::Setting p(1);
        std::vector<primitives::Setting> v;
        v.push_back(1);
        v.push_back(1.5);
        p = v;
        p = 1;
    }

    {
        std::vector<primitives::Setting> v;
        v.push_back(1);
        v.push_back(1.5);
        primitives::Setting p(v);
        p = v;
        p = 1;
    }
}

TEST_CASE("Checking settings", "[settings]")
{
    //using namespace primitives;

    // strings

    // bare and basic
    CHECK_NOTHROW(getSettings().getLocalSettings().load("a=b"s));
    CHECK_NOTHROW(getSettings().getLocalSettings().load("a=b\n"s));
    CHECK_NOTHROW(getSettings().getLocalSettings().load("a=b\r\n"s));
    CHECK_NOTHROW(getSettings().getLocalSettings().load("a = b"s));
    CHECK_NOTHROW(getSettings().getLocalSettings().load(" a = b "s));
    CHECK_NOTHROW(getSettings().getLocalSettings().load("a = \"\"\"b\"\"\""s));
    CHECK_NOTHROW(getSettings().getLocalSettings().load("a = \"b\""s));
    CHECK_NOTHROW(getSettings().getLocalSettings().load("a = \"\"\" b \n \"\"\""s));
    CHECK_NOTHROW(getSettings().getLocalSettings().load("a = \"\"\" b \r\n \"\"\""s));
    CHECK_NOTHROW(getSettings().getLocalSettings().load("a = \"\"\" b \" \r\n \"\"\""s));
    CHECK_NOTHROW(getSettings().getLocalSettings().load("a = \"\"\" b \"\" \r\n \"\"\""s));
    CHECK_NOTHROW(getSettings().getLocalSettings().load("a = \"\"\" b \"\" ''' \r\n \"\"\""s));
    CHECK_NOTHROW(getSettings().getLocalSettings().load("a = \"\"\" b \"\" ' '' \r\n \"\"\""s));
    CHECK_NOTHROW(getSettings().getLocalSettings().load("\r\n\t  a = \"\"\" b \"\" ' '' \r\n \"\"\"  "s));
    CHECK_NOTHROW(getSettings().getLocalSettings().load("\r\n\t  a = \"\"\" b \"\" ' '' \r\n \"\"\"  \r\n\t  "s));
    CHECK_THROWS(getSettings().getLocalSettings().load("a = \"\"\" b \"\"\" \r\n \"\"\""s));
    CHECK_THROWS(getSettings().getLocalSettings().load("a = \"\"\" b \r \"\"\""s));
    CHECK_THROWS(getSettings().getLocalSettings().load("a = \"\"\"\rb\"\"\""s));
    CHECK_THROWS(getSettings().getLocalSettings().load("a\n.b=b"s));
    CHECK_THROWS(getSettings().getLocalSettings().load("a.\nb=b"s));
    CHECK_THROWS(getSettings().getLocalSettings().load("a.b\n=b"s));
    CHECK_THROWS(getSettings().getLocalSettings().load("a.b=\nb"s));
    CHECK_THROWS(getSettings().getLocalSettings().load("a=b\r"s));
    CHECK_THROWS(getSettings().getLocalSettings().load("\r"s));
    CHECK_THROWS(getSettings().getLocalSettings().load("\v"s));
    CHECK_THROWS(getSettings().getLocalSettings().load("\f"s));
    CHECK_NOTHROW(getSettings().getLocalSettings().load(" "s));
    CHECK_NOTHROW(getSettings().getLocalSettings().load("\t"s));
    CHECK_NOTHROW(getSettings().getLocalSettings().load("\n"s));
    CHECK_NOTHROW(getSettings().getLocalSettings().load("\r\n"s));
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
            a = """\


                    bc\ \


                \d\
""")";
        CHECK_THROWS(getSettings().getLocalSettings().load(cfg));

        cfg = R"(
            a = """\


                    bc\\ \


                \\d\
""")";
        CHECK_NOTHROW(getSettings().getLocalSettings().load(cfg));
        CHECK(getSettings().getLocalSettings()["a"] == "bc\\ \\d");

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
    CHECK_NOTHROW(getSettings().getLocalSettings().load("\r\n\t  a = ''' b \"\" \" \"\" \r\n '''  "s));
    CHECK_NOTHROW(getSettings().getLocalSettings().load("\r\n\t  a = ''' b \"\" \" \"\" \r\n '''  \r\n\t  "s));
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
            "\r\n\t  e.b.c = \"\"\" b \"\" ' '' \r\n \"\"\"  \r\n\t  "
            " a=b\n"
            "b=a\r\n"
            "b.c.d=a\r\n"
            "b . c . d . e=abc\r\n"
            "\r\n\t  a.b.c.d = ''' b \"\" \" \"\" \r\n '''  \r\n\t  "
        ;
        CHECK_NOTHROW(getSettings().getLocalSettings().load(cfg));
        CHECK(getSettings().getLocalSettings()["a"] == "b");
        CHECK(getSettings().getLocalSettings()["b"] == "a");
        CHECK(getSettings().getLocalSettings()["b.c.d"] == "a");
        CHECK(getSettings().getLocalSettings()["b.c.d.e"] == "abc");
    }

    // table
    CHECK_NOTHROW(getSettings().getLocalSettings().load("[a]"s));
    CHECK_THROWS(getSettings().getLocalSettings().load("[a][b]"s));
    CHECK_NOTHROW(getSettings().getLocalSettings().load("[a]\n[b]"s));

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
        String cfg =
            "#\n"
            "[a]\n"
            "[b]\n"
            "b=c\n"
            "[b.b]\n"
            "b=d\n"
            "[a.b.c]\n"
            "d=e\n"
            "#"
            ;
        CHECK_NOTHROW(getSettings().getLocalSettings().load(cfg));
        CHECK(getSettings().getLocalSettings()["b.b"] == "c");
        CHECK(getSettings().getLocalSettings()["b.b.b"] == "d");
        CHECK(getSettings().getLocalSettings()["a.b.c.d"] == "e");
    }

    {
        String cfg =
            "[a]\n"
            "[b][a]\n"
            "b=c\n"
            "[b.b]\n"
            "b=d\n"
            "[a.b.c] # comment\n"
            "d=e\n"
            ;
        CHECK_THROWS(getSettings().getLocalSettings().load(cfg));
    }

    // non-string values

    // rhs bare!
    CHECK_NOTHROW(getSettings().getLocalSettings().load("a=5-"s));
    CHECK_NOTHROW(getSettings().getLocalSettings().load("a=5+"s));
    CHECK_NOTHROW(getSettings().getLocalSettings().load("a=5a"s));
    CHECK_NOTHROW(getSettings().getLocalSettings().load("a=5x"s));
    CHECK_NOTHROW(getSettings().getLocalSettings().load("a=0x"s));
    CHECK_NOTHROW(getSettings().getLocalSettings().load("a=00x"s));
    CHECK_NOTHROW(getSettings().getLocalSettings().load("a=-0xg"s));
    CHECK_NOTHROW(getSettings().getLocalSettings().load("a=-0x0"s));
    CHECK_NOTHROW(getSettings().getLocalSettings().load("a=08"s));
    CHECK_NOTHROW(getSettings().getLocalSettings().load("a=0b2"s));
    CHECK_NOTHROW(getSettings().getLocalSettings().load("a=-0xf"s));
    CHECK_NOTHROW(getSettings().getLocalSettings().load("a=+0xf"s));
    CHECK_NOTHROW(getSettings().getLocalSettings().load("a=0b_"s));
    CHECK_NOTHROW(getSettings().getLocalSettings().load("a=0b0_"s));
    CHECK_NOTHROW(getSettings().getLocalSettings().load("a=0b_0"s));
    CHECK_NOTHROW(getSettings().getLocalSettings().load("a=0b_0_"s));

    // bin
    CHECK_NOTHROW(getSettings().getLocalSettings().load("a=0b0"s));
    CHECK_NOTHROW(getSettings().getLocalSettings().load("a=0b000"s));
    CHECK_NOTHROW(getSettings().getLocalSettings().load("a=0b1"s));
    CHECK_NOTHROW(getSettings().getLocalSettings().load("a=0b01"s));
    CHECK_NOTHROW(getSettings().getLocalSettings().load("a=0b01010110011010010"s));
    CHECK_NOTHROW(getSettings().getLocalSettings().load("a=0b0_0"s));
    CHECK_NOTHROW(getSettings().getLocalSettings().load("a=0b1"s));
    CHECK_NOTHROW(getSettings().getLocalSettings().load("a=0b01"s));
    CHECK_NOTHROW(getSettings().getLocalSettings().load("a=0b01010110011010010"s));
    CHECK_NOTHROW(getSettings().getLocalSettings().load("a=0b0''1"s));

    // hex
    CHECK_NOTHROW(getSettings().getLocalSettings().load("a=0xf"s));
    CHECK_NOTHROW(getSettings().getLocalSettings().load("a=0x0"s));

    {
        String cfg =
            "a=5\n"
            //"b=5.5\n"
            "c=false\n"
            "d=true\n"
            "e=-5\n"
            "f=01_0  \n"
            "ff=  0_1_0  \n"
            "ff2=  0  1_0 * \n"
            "ff3=  0o_1_0  \n"
            "ff4=  00_1_0  \n"
            "ff5=  0o0_1_0  \n"
            "g=0b0_1'0   \n"
            "h= 0xF'F \n"
            "i= 1'000'000'000 \n"
            "j= 1'0'0'0'0'0'0'0'0'0 \n"
            "k=a=0X1.BC70A3D70A3D7P+6\n"
            "l:0X1.BC70A3D70A3D7P+6\n"
            ;
        CHECK_NOTHROW(getSettings().getLocalSettings().load(cfg));
        CHECK(getSettings().getLocalSettings()["a"] == 5);
        //CHECK(getSettings().getLocalSettings()["b"] == 5.5);
        CHECK(getSettings().getLocalSettings()["c"] == false);
        CHECK(getSettings().getLocalSettings()["d"] == true);
        CHECK(getSettings().getLocalSettings()["e"] == -5);
        CHECK(getSettings().getLocalSettings()["ff"] == 10);
        CHECK(getSettings().getLocalSettings()["ff2"] == "0  1_0 *");
        CHECK(getSettings().getLocalSettings()["ff3"] == "0o_1_0");
        CHECK(getSettings().getLocalSettings()["ff4"] == 8);
        CHECK(getSettings().getLocalSettings()["ff5"] == 8);
        CHECK(getSettings().getLocalSettings()["f"] == 8);
        CHECK(getSettings().getLocalSettings()["g"] == 2);
        CHECK(getSettings().getLocalSettings()["h"] == 255);
        CHECK(getSettings().getLocalSettings()["i"] == 1000000000);
        CHECK(getSettings().getLocalSettings()["j"] == 1000000000);
        CHECK(getSettings().getLocalSettings()["k"] == "a=0X1.BC70A3D70A3D7P+6");
        CHECK(getSettings().getLocalSettings()["l"] == 111.11);
        CHECK_THROWS(getSettings().getLocalSettings()["a"] == "x");
    }

    // float
    CHECK_NOTHROW(getSettings().getLocalSettings().load("a=111.11"s));
    CHECK_NOTHROW(getSettings().getLocalSettings().load("a=-2.22"s));
    CHECK_NOTHROW(getSettings().getLocalSettings().load("a=1.18973e+49"s));
    CHECK_NOTHROW(getSettings().getLocalSettings().load("a=1.18973e+4932zzz"s));
    CHECK_NOTHROW(getSettings().getLocalSettings().load("a=inf"s));
    CHECK_NOTHROW(getSettings().getLocalSettings().load("a=-inf"s));
    CHECK_NOTHROW(getSettings().getLocalSettings().load("a=+infinity"s));
    CHECK_NOTHROW(getSettings().getLocalSettings().load("a=nan"s));
    CHECK_NOTHROW(getSettings().getLocalSettings().load("a=-nAN"s));
    CHECK_NOTHROW(getSettings().getLocalSettings().load("a=+nan(2)"s));

    {
        String cfg = R"(
    array5 = [
        "Whitespace", "is", "ignored"
    ]
)";
        getSettings().getLocalSettings().load(cfg);
    }

    // from file
    //CHECK_NOTHROW(getSettings().getLocalSettings().load(path(__FILE__).parent_path() / "test.cfg"));

    getSettings().getLocalSettings().clear();
    getSettings().getLocalSettings().load(path(__FILE__).parent_path() / "test.yml");
    getSettings().getLocalSettings().save("test2.yml");
    // TODO: add more setting checks
}

TEST_CASE("Checking command line", "[cl]")
{
    using namespace primitives;

    cl::option<int> a{"x"};

    Strings args = {
        "prog",
        "1",
        "@@" + (path(__FILE__).parent_path() / "test.yml").string(),
        "@" + (path(__FILE__).parent_path() / "test.rsp").string(),
        "2",
    };

    ::primitives::cl::parseCommandLineOptions(args);
}

#include <llvm/Support/CommandLine.h>

#include <any>

int main(int argc, char **argv)
{
    {
        using namespace llvm;

        cl::opt<std::string> OutputFilename("o", cl::desc("Specify output filename"), cl::value_desc("filename"));
        cl::opt<bool> b1{ "b" };
        cl::SubCommand test("test");
        cl::opt<bool> b2{ "d", cl::sub(test) };
        cl::ParseCommandLineOptions(argc, argv);

        //std::ofstream Output(OutputFilename.c_str());

        //return 0;
    }

    namespace st = sw::settings;

    st::setting<int> myval("myval", st::do_not_save(), st::init(5));
    myval = 10;
    int a = myval;
    auto &ssss = getSettings().getLocalSettings()["myval"].as<int64_t>();
    ssss = 2;

    std::any x;
    x = 5;
    auto x2 = std::any_cast<int>(x);
    auto &t3 = x.type();
    x = 5LL;
    auto x3 = std::any_cast<long long>(x);
    auto &t4 = x.type();


    auto &t1 = typeid(int);
    t1.hash_code();
    auto &t2 = typeid(long long);

    t1 == t2;
    t1 == t3;
    t3 == t4;

    ::argc = argc;
    ::argv = argv;

    Catch::Session s;
    //while (1)
    s.run(argc, argv);

    return 0;
}
