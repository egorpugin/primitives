// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <primitives/filesystem.h>
#include <primitives/version.h>

#include <chrono>
#include <iostream>

#define CATCH_CONFIG_RUNNER
#include <catch.hpp>

TEST_CASE("Checking versions", "[version]")
{
    using namespace primitives::version;

    // valid branches
    REQUIRE_NOTHROW(Version("a"));
    REQUIRE_NOTHROW(Version("ab"));
    REQUIRE_NOTHROW(Version("abc123___"));
    CHECK_THROWS(Version("1abc123___"));
    REQUIRE_NOTHROW(Version("abc123___-"));
    REQUIRE_NOTHROW(Version("a-a"));
    CHECK_THROWS(Version("1..1"));
    CHECK_THROWS(Version("1.1-2..2"));
    CHECK_THROWS(Version("1.1-2-2"));
    CHECK_THROWS(Version("1.1-"));
    CHECK_THROWS(Version(""));
    CHECK_THROWS(Version("-"));
    CHECK_THROWS(Version("."));

    REQUIRE_NOTHROW(Version("1"));
    REQUIRE_NOTHROW(Version("1-alpha1")); // with extra
    REQUIRE_NOTHROW(Version("1.2"));
    REQUIRE_NOTHROW(Version("1.2-rc2.3.a")); // with extra
    CHECK_THROWS(Version("1.2--rc2.3.a")); // with extra
    CHECK_THROWS(Version("1.2-rc2.3.a-")); // with extra
    CHECK_THROWS(Version("1.2-rc2.3.-a")); // with extra
    REQUIRE_NOTHROW(Version("1.2.3"));
    REQUIRE_NOTHROW(Version("1.2.3.4"));
    CHECK_THROWS(Version("1.2.*"));
    CHECK_THROWS(Version("1.2.x"));
    CHECK_THROWS(Version("1.2.X"));
    CHECK_THROWS(Version("1.2.3.4.5"));
    CHECK_THROWS(Version("1.2.3.4.5.6"));
    CHECK_THROWS(Version("1.2.3.4.5.6.7"));
    CHECK_THROWS(Version(0, "-rc2.3._a_"));

    {
        Version v;
        CHECK(v.getMajor() == 0);
        CHECK(v.getMinor() == 0);
        CHECK(v.getPatch() == 1);
        CHECK(v.getTweak() == 0);
        v.incrementVersion();
        CHECK(v.getPatch() == 2);
        v.incrementVersion();
        CHECK(v.getPatch() == 3);
        v.decrementVersion();
        v.decrementVersion();
        CHECK(v.getPatch() == 1);
        v.decrementVersion();
        CHECK(v.getPatch() == 1);
        v.decrementVersion();
        v.decrementVersion();
        CHECK(v.getPatch() == 1);
        CHECK(v.isVersion());
        CHECK_FALSE(v.isBranch());
    }

    {
        Version v("0");
        CHECK(v.getMajor() == 0);
        CHECK(v.getMinor() == 0);
        CHECK(v.getPatch() == 1);
        CHECK(v.getTweak() == 0);
        v.decrementVersion();
        v.decrementVersion();
        v.decrementVersion();
        CHECK(v.getMajor() == 0);
        CHECK(v.getMinor() == 0);
        CHECK(v.getPatch() == 1);
        CHECK(v.getTweak() == 0);
    }

    {
        Version v("0.0.0");
        CHECK(v.getMajor() == 0);
        CHECK(v.getMinor() == 0);
        CHECK(v.getPatch() == 0);
        CHECK(v.getTweak() == 1);
        v.decrementVersion();
        v.decrementVersion();
        v.decrementVersion();
        CHECK(v.getMajor() == 0);
        CHECK(v.getMinor() == 0);
        CHECK(v.getPatch() == 0);
        CHECK(v.getTweak() == 1);
    }

    {
        Version v = Version::min();
        CHECK(v.getMajor() == 0);
        CHECK(v.getMinor() == 0);
        CHECK(v.getPatch() == 1);
        CHECK(v.getTweak() == 0);
        v.decrementVersion();
        v.decrementVersion();
        v.decrementVersion();
        CHECK(v.getMajor() == 0);
        CHECK(v.getMinor() == 0);
        CHECK(v.getPatch() == 1);
        CHECK(v.getTweak() == 0);
    }

    {
        Version v1(0, 0, 0);
        Version v = Version::min(v1);
        CHECK(v.getMajor() == 0);
        CHECK(v.getMinor() == 0);
        CHECK(v.getPatch() == 0);
        CHECK(v.getTweak() == 1);
        v.decrementVersion();
        v.decrementVersion();
        v.decrementVersion();
        CHECK(v.getMajor() == 0);
        CHECK(v.getMinor() == 0);
        CHECK(v.getPatch() == 0);
        CHECK(v.getTweak() == 1);
    }

    {
        Version v = Version::max();
        CHECK(v.getMajor() == Version::maxNumber());
        CHECK(v.getMinor() == Version::maxNumber());
        CHECK(v.getPatch() == Version::maxNumber());
        CHECK(v.getTweak() == 0);
        v.decrementVersion();
        v.decrementVersion();
        v.decrementVersion();
        CHECK(v.getMajor() == Version::maxNumber());
        CHECK(v.getMinor() == Version::maxNumber());
        CHECK(v.getPatch() == Version::maxNumber() - 3);
        CHECK(v.getTweak() == 0);
    }

    {
        Version v1(0, 0, 0);
        Version v = Version::max(v1);
        CHECK(v.getMajor() == Version::maxNumber());
        CHECK(v.getMinor() == Version::maxNumber());
        CHECK(v.getPatch() == Version::maxNumber());
        CHECK(v.getTweak() == Version::maxNumber());
        v.decrementVersion();
        v.decrementVersion();
        v.decrementVersion();
        CHECK(v.getMajor() == Version::maxNumber());
        CHECK(v.getMinor() == Version::maxNumber());
        CHECK(v.getPatch() == Version::maxNumber());
        CHECK(v.getTweak() == Version::maxNumber() - 3);
    }

    {
        Version v("00000000000");
        CHECK(v.getMajor() == 0);
        CHECK(v.getMinor() == 0);
        CHECK(v.getPatch() == 1);
        CHECK(v.getTweak() == 0);
    }

    {
        Version v("00000000000.00000000.000001");
        CHECK(v.getMajor() == 0);
        CHECK(v.getMinor() == 0);
        CHECK(v.getPatch() == 1);
        CHECK(v.getTweak() == 0);
    }

    {
        Version v(0);
        CHECK(v.getMajor() == 0);
        CHECK(v.getMinor() == 0);
        CHECK(v.getPatch() == 1);
        CHECK(v.getTweak() == 0);
    }

    {
        Version v("0.0");
        CHECK(v.getMajor() == 0);
        CHECK(v.getMinor() == 0);
        CHECK(v.getPatch() == 1);
        CHECK(v.getTweak() == 0);
    }

    {
        Version v(0,0);
        CHECK(v.getMajor() == 0);
        CHECK(v.getMinor() == 0);
        CHECK(v.getPatch() == 1);
        CHECK(v.getTweak() == 0);
    }

    {
        Version v("0.0.0");
        CHECK(v.getMajor() == 0);
        CHECK(v.getMinor() == 0);
        CHECK(v.getPatch() == 0);
        CHECK(v.getTweak() == 1);
        v.incrementVersion();
        CHECK(v.getTweak() == 2);
        v.incrementVersion();
        CHECK(v.getTweak() == 3);
        v.decrementVersion();
        v.decrementVersion();
        CHECK(v.getTweak() == 1);
        v.decrementVersion();
        CHECK(v.getTweak() == 1);
        v.decrementVersion();
        v.decrementVersion();
        CHECK(v.getTweak() == 1);
    }

    {
        Version v(0, 0, 0);
        CHECK(v.getMajor() == 0);
        CHECK(v.getMinor() == 0);
        CHECK(v.getPatch() == 0);
        CHECK(v.getTweak() == 1);
    }

    {
        Version v("1");
        CHECK(v.getMajor() == 1);
        CHECK(v.getMinor() == 0);
        CHECK(v.getPatch() == 0);
        CHECK(v.getTweak() == 0);
        v.incrementVersion();
        CHECK(v.getPatch() == 1);
        v.decrementVersion();
        CHECK(v.getPatch() == 0);
        v.decrementVersion();
        CHECK(v.getMinor() == v.maxNumber());
        CHECK(v.getPatch() == v.maxNumber());
        v.decrementVersion();
        CHECK(v.getPatch() == v.maxNumber() - 1);
    }

    {
        Version v(1);
        CHECK(v.getMajor() == 1);
        CHECK(v.getMinor() == 0);
        CHECK(v.getPatch() == 0);
        CHECK(v.getTweak() == 0);
    }

    {
        Version v("1.2");
        CHECK(v.getMajor() == 1);
        CHECK(v.getMinor() == 2);
        CHECK(v.getPatch() == 0);
        CHECK(v.getTweak() == 0);
        v.decrementVersion();
        CHECK(v.getMinor() == 1);
        CHECK(v.getPatch() == v.maxNumber());
    }

    {
        Version v(1, 2);
        CHECK(v.getMajor() == 1);
        CHECK(v.getMinor() == 2);
        CHECK(v.getPatch() == 0);
        CHECK(v.getTweak() == 0);
    }

    {
        Version v("1.2.3");
        CHECK(v.getMajor() == 1);
        CHECK(v.getMinor() == 2);
        CHECK(v.getPatch() == 3);
        CHECK(v.getTweak() == 0);
    }

    {
        Version v(1,2,3);
        CHECK(v.getMajor() == 1);
        CHECK(v.getMinor() == 2);
        CHECK(v.getPatch() == 3);
        CHECK(v.getTweak() == 0);
    }

    {
        Version v("1.2.3.4");
        CHECK(v.getMajor() == 1);
        CHECK(v.getMinor() == 2);
        CHECK(v.getPatch() == 3);
        CHECK(v.getTweak() == 4);
    }

    {
        Version v(1,2,3,4);
        CHECK(v.getMajor() == 1);
        CHECK(v.getMinor() == 2);
        CHECK(v.getPatch() == 3);
        CHECK(v.getTweak() == 4);
    }

    {
        Version v("1-alpha1");
        CHECK(v.getMajor() == 1);
        CHECK(v.getMinor() == 0);
        CHECK(v.getPatch() == 0);
        CHECK(v.getTweak() == 0);
        CHECK(v.getExtra()[0] == "alpha1");
    }

    {
        Version v("1.2-rc2.3._a_");
        CHECK(v.getMajor() == 1);
        CHECK(v.getMinor() == 2);
        CHECK(v.getPatch() == 0);
        CHECK(v.getTweak() == 0);
        CHECK(v.getExtra()[0] == "rc2");
        CHECK(v.getExtra()[1] == "3");
        CHECK(v.getExtra()[2] == "_a_");
    }

    {
        Version v("1.2.3.4-alpha1");
        CHECK(v.getMajor() == 1);
        CHECK(v.getMinor() == 2);
        CHECK(v.getPatch() == 3);
        CHECK(v.getTweak() == 4);
        CHECK(v.getExtra()[0] == "alpha1");
    }

    {
        Version v("1.2.3.4-rc2.3._a_");
        CHECK(v.getMajor() == 1);
        CHECK(v.getMinor() == 2);
        CHECK(v.getPatch() == 3);
        CHECK(v.getTweak() == 4);
        CHECK(v.getExtra()[0] == "rc2");
        CHECK(v.getExtra()[1] == "3");
        CHECK(v.getExtra()[2] == "_a_");
    }

    {
        Version v("a");
        CHECK(v.getBranch() == "a");
        CHECK(v.isBranch());
        CHECK_FALSE(v.isVersion());
    }

    {
        Version v(0, "rc2.3._a_");
        CHECK(v.getMajor() == 0);
        CHECK(v.getMinor() == 0);
        CHECK(v.getPatch() == 1);
        CHECK(v.getTweak() == 0);
        CHECK(v.getExtra()[0] == "rc2");
        CHECK(v.getExtra()[1] == "3");
        CHECK(v.getExtra()[2] == "_a_");
    }

    {
        Version v(0,0,0, "rc2.3._a_");
        CHECK(v.getMajor() == 0);
        CHECK(v.getMinor() == 0);
        CHECK(v.getPatch() == 0);
        CHECK(v.getTweak() == 1);
        CHECK(v.getExtra()[0] == "rc2");
        CHECK(v.getExtra()[1] == "3");
        CHECK(v.getExtra()[2] == "_a_");
    }
}

TEST_CASE("Checking version ranges", "[range]")
{
    using namespace primitives::version;

    // RANGE
    {
        VersionRange vr("*");
        auto s = vr.toString();
        CHECK(s == "*");
        CHECK(VersionRange("x").toString() == "*");
        CHECK(VersionRange("X").toString() == "*");
    }

    {
        VersionRange vr("1");
        auto s = vr.toString();
        CHECK(s == ">=1.0.0 <2.0.0");
    }

    {
        VersionRange vr("1.2");
        auto s = vr.toString();
        CHECK(s == ">=1.2.0 <1.3.0");
    }

    {
        VersionRange vr("1.2.3");
        auto s = vr.toString();
        CHECK(s == "1.2.3");
    }

    {
        VersionRange vr("1.2.3.4");
        auto s = vr.toString();
        CHECK(s == "1.2.3.4");
    }

    {
        VersionRange vr("1.0.1");
        auto s = vr.toString();
        CHECK(s == "1.0.1");
    }

    {
        VersionRange vr("1.0.0.2");
        auto s = vr.toString();
        CHECK(s == "1.0.0.2");
    }

    // CMP
    {
        VersionRange vr("> 1");
        auto s = vr.toString();
        CHECK(s == ">=1.0.1");
    }

    {
        VersionRange vr(">1.2");
        auto s = vr.toString();
        CHECK(s == ">=1.2.1");
    }

    {
        VersionRange vr(">1.2.3");
        auto s = vr.toString();
        CHECK(s == ">=1.2.4");
    }

    {
        VersionRange vr(">1.2.3.4");
        auto s = vr.toString();
        CHECK(s == ">=1.2.3.5");
    }

    {
        VersionRange vr("> 1.0.0");
        auto s = vr.toString();
        CHECK(s == ">=1.0.1");
    }

    {
        VersionRange vr(">= 1.0.0");
        auto s = vr.toString();
        CHECK(s == ">=1.0.0");
    }

    {
        VersionRange vr("< 1");
        auto s = vr.toString();
        CHECK(s == "<1.0.0");
    }

    {
        VersionRange vr("<1.2");
        auto s = vr.toString();
        CHECK(s == "<1.2.0");
    }

    {
        VersionRange vr("<1.2.3");
        auto s = vr.toString();
        CHECK(s == "<=1.2.2");
    }

    {
        VersionRange vr("<1.2.3.4");
        auto s = vr.toString();
        CHECK(s == "<=1.2.3.3");
    }

    {
        VersionRange vr("< 1.0.0");
        auto s = vr.toString();
        CHECK(s == "<1.0.0");
    }

    {
        VersionRange vr("<= 1");
        auto s = vr.toString();
        CHECK(s == "<=1.0.0");
    }

    {
        VersionRange vr("<= 1.0.0");
        auto s = vr.toString();
        CHECK(s == "<=1.0.0");
    }

    {
        VersionRange vr("=1"s);
        CHECK(vr.toString() == "1.0.0");
        CHECK(VersionRange("= 1").toString() == "1.0.0");
    }

    {
        VersionRange vr("!= 1");
        CHECK(vr.toString() == "<1.0.0 || >=1.0.1");
    }

    // TILDE
    {
        VersionRange vr("~ 1");
        auto s = vr.toString();
        CHECK(s == ">=1.0.0 <2.0.0");
    }

    {
        VersionRange vr("~ 1.2");
        auto s = vr.toString();
        CHECK(s == ">=1.2.0 <1.3.0");
    }

    {
        VersionRange vr("~1.2.3");
        auto s = vr.toString();
        CHECK(s == ">=1.2.3 <1.3.0");
    }

    {
        VersionRange vr("~1.2.3.4");
        auto s = vr.toString();
        CHECK(s == ">=1.2.3.4 <1.3.0.0");
        // maybe this variant?
        //CHECK(s == ">=1.2.3.4 <1.2.4.0");
    }

    // CARET
    {
        VersionRange vr("^ 1");
        auto s = vr.toString();
        CHECK(s == ">=1.0.0 <2.0.0");
    }

    {
        VersionRange vr("^ 1.2");
        auto s = vr.toString();
        CHECK(s == ">=1.2.0 <2.0.0");
    }

    {
        VersionRange vr("^1.2.3");
        auto s = vr.toString();
        CHECK(s == ">=1.2.3 <2.0.0");
    }

    {
        VersionRange vr("^1.2.3.4");
        auto s = vr.toString();
        CHECK(s == ">=1.2.3.4 <2.0.0.0");
    }

    {
        VersionRange vr("^ 0");
        auto s = vr.toString();
        CHECK(s == "<1.0.0");
    }

    {
        VersionRange vr("^ 0.0");
        auto s = vr.toString();
        CHECK(s == "<0.1.0");
    }

    {
        VersionRange vr("^ 0.2");
        auto s = vr.toString();
        CHECK(s == ">=0.2.0 <0.3.0");
    }

    {
        VersionRange vr("^ 0.2.3");
        auto s = vr.toString();
        CHECK(s == ">=0.2.3 <0.3.0");
    }

    {
        VersionRange vr("^0.0.3");
        auto s = vr.toString();
        CHECK(s == "0.0.3");
    }

    {
        VersionRange vr("^0.0.3.0");
        auto s = vr.toString();
        CHECK(s == "0.0.3");
    }

    {
        VersionRange vr("^0.0.0.4");
        auto s = vr.toString();
        CHECK(s == "0.0.0.4");
    }

    // HYPHEN
    {
        VersionRange vr("1 - 2");
        auto s = vr.toString();
        CHECK(s == ">=1.0.0 <3.0.0");
    }

    {
        VersionRange vr("1-2");
        CHECK(vr.toString() == ">=1.0.0-2 <2.0.0-2");
    }

    {
        VersionRange vr("1-2 - 2-1");
        CHECK(vr.toString() == ">=1.0.0-2 <3.0.0-1");
    }

    {
        VersionRange vr("1.2 - 2");
        CHECK(vr.toString() == ">=1.2.0 <3.0.0");
    }

    {
        VersionRange vr("1.2 - 2.3");
        CHECK(vr.toString() == ">=1.2.0 <2.4.0");
    }

    {
        VersionRange vr("1.2.3 - 2");
        CHECK(vr.toString() == ">=1.2.3 <3.0.0");
    }

    {
        VersionRange vr("1.2.3 - 2.3.4");
        CHECK(vr.toString() == ">=1.2.3 <=2.3.4");
    }

    {
        VersionRange vr("1.2.3.4 - 2");
        CHECK(vr.toString() == ">=1.2.3.4 <3.0.0.0");
    }

    {
        VersionRange vr("1.2.3.4 - 2.3.4.5");
        CHECK(vr.toString() == ">=1.2.3.4 <=2.3.4.5");
    }

    // NORMALIZE
    {
        VersionRange vr(">7 >8 >9");
        CHECK(vr.toString() == ">=7.0.1");
    }

    {
        VersionRange vr(">=7 >8 >9");
        CHECK(vr.toString() == ">=7.0.0");
    }

    {
        VersionRange vr(">7 >8 <10");
        CHECK(vr.toString() == "*");
    }

    {
        VersionRange vr(">7 ||>8 || >9");
        CHECK(vr.toString() == ">=7.0.1");
    }

    {
        VersionRange vr(">=7 || >8 || >9");
        CHECK(vr.toString() == ">=7.0.0");
    }

    {
        VersionRange vr(">7||>8||<10");
        CHECK(vr.toString() == "*");
    }

    // from simple to complex
    CHECK(VersionRange("1").toString() == ">=1.0.0 <2.0.0");
    CHECK(VersionRange("1.0.0").toString() == "1.0.0");
    CHECK(VersionRange("1.0.0.01").toString() == "1.0.0.1");
    CHECK(VersionRange("1 - 2").toString() == ">=1.0.0 <3.0.0");
    CHECK(VersionRange("1.2 - 2").toString() == ">=1.2.0 <3.0.0");
    CHECK(VersionRange("1.2.3 - 2").toString() == ">=1.2.3 <3.0.0");
    CHECK(VersionRange("1.2.3.4 - 2").toString() == ">=1.2.3.4 <3.0.0.0");

    //"^4.8.0 || ^5.7.0 || ^6.2.2 || >=8.0.0"
}

int main(int argc, char **argv)
try
{
    Catch::Session().run(argc, argv);

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
