// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

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
    }

    {
        Version v(0, 0, 0);
        CHECK(v.getMajor() == 0);
        CHECK(v.getMinor() == 0);
        CHECK(v.getPatch() == 1);
        CHECK(v.getTweak() == 0);
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
        Version v("1.2-1");
        CHECK(v.getMajor() == 1);
        CHECK(v.getMinor() == 2);
        CHECK(v.getPatch() == 0);
        CHECK(v.getTweak() == 0);
        CHECK(v.getExtra()[0] == 1);
    }

    {
        Version v("1.2-rc2.3._a_");
        CHECK(v.getMajor() == 1);
        CHECK(v.getMinor() == 2);
        CHECK(v.getPatch() == 0);
        CHECK(v.getTweak() == 0);
        CHECK(v.getExtra()[0] == "rc2");
        CHECK(v.getExtra()[1] == 3);
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
        CHECK(v.getExtra()[1] == 3);
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
        CHECK(v.getExtra()[1] == 3);
        CHECK(v.getExtra()[2] == "_a_");
    }

    {
        Version v(0,0,0, "rc2.3._a_");
        CHECK(v.getMajor() == 0);
        CHECK(v.getMinor() == 0);
        CHECK(v.getPatch() == 1);
        CHECK(v.getTweak() == 0);
        CHECK(v.getExtra()[0] == "rc2");
        CHECK(v.getExtra()[1] == 3);
        CHECK(v.getExtra()[2] == "_a_");
    }

    {
        Version v(1,2,0);
        CHECK(v.getMajor() == 1);
        CHECK(v.getMinor() == 2);
        CHECK(v.getPatch() == 0);
        CHECK(v.getTweak() == 0);
        v.incrementVersion(2);
        CHECK(v.getMajor() == 1);
        CHECK(v.getMinor() == 3);
        CHECK(v.getPatch() == 0);
        CHECK(v.getTweak() == 0);
        v.incrementVersion(1);
        CHECK(v.getMajor() == 2);
        CHECK(v.getMinor() == 3);
        CHECK(v.getPatch() == 0);
        CHECK(v.getTweak() == 0);
        v.incrementVersion();
        CHECK(v.getMajor() == 2);
        CHECK(v.getMinor() == 3);
        CHECK(v.getPatch() == 1);
        CHECK(v.getTweak() == 0);
        v.incrementVersion(4);
        CHECK(v.getMajor() == 2);
        CHECK(v.getMinor() == 3);
        CHECK(v.getPatch() == 1);
        CHECK(v.getTweak() == 1);
        v.decrementVersion();
        CHECK(v.getTweak() == 0);
        v.decrementVersion();
        CHECK(v.getPatch() == 0);
        CHECK(v.getTweak() == Version::maxNumber());
    }

    {
        Version v(1, 2, 0);
        CHECK(v.getMajor() == 1);
        CHECK(v.getMinor() == 2);
        CHECK(v.getPatch() == 0);
        CHECK(v.getTweak() == 0);
        v.incrementVersion(2);
        CHECK(v.getMajor() == 1);
        CHECK(v.getMinor() == 3);
        CHECK(v.getPatch() == 0);
        CHECK(v.getTweak() == 0);
        v.incrementVersion(1);
        CHECK(v.getMajor() == 2);
        CHECK(v.getMinor() == 3);
        CHECK(v.getPatch() == 0);
        CHECK(v.getTweak() == 0);
        v.incrementVersion();
        CHECK(v.getMajor() == 2);
        CHECK(v.getMinor() == 3);
        CHECK(v.getPatch() == 1);
        CHECK(v.getTweak() == 0);
        v.decrementVersion(4);
        CHECK(v.getPatch() == 0);
        CHECK(v.getTweak() == Version::maxNumber());
    }

    CHECK(Version(0, 0, 0, "rc2.3._a_") > Version(0, 0, 0, "rc1.3._a_"));
    CHECK(Version(0, 0, 0, "rc2.3._a_") > Version(0, 0, 0, "beta.3._a_"));
    CHECK(Version(0, 0, 0, "rc2.3._a_") > Version(0, 0, 0, "alpha.3._a_"));
    CHECK(Version(0, 0, 0, "rc2.3._a_") < Version(0, 0, 0, "rc3.3._a_"));
    CHECK(Version(0, 0, 0, "rc.2.3._a_") < Version(0, 0, 0, "rc.3.3._a_"));
    CHECK(Version(0, 0, 0, "rc.2.3._a_") == Version(0, 0, 0, "rc.2.3._a_"));

    CHECK(Version(0, 0, 0, "1") == Version(0, 0, 0, "1"));
    CHECK(Version(0, 0, 0, "1") < Version(0, 0, 0, "2"));
    CHECK(Version(0, 0, 0, "1.1") < Version(0, 0, 0, "2.1"));
    CHECK(Version(0, 0, 0, "1.1") < Version(0, 0, 0, "1.2"));
    CHECK(Version(0, 0, 0, "1.1") == Version(0, 0, 0, "1.1"));
    CHECK(Version(0, 0, 0, "1.1.a") < Version(0, 0, 0, "1.1.z"));

    CHECK(Version(0).format("{ML}") == "A");
    CHECK(Version(1).format("{ML}") == "B");
    CHECK(Version(2).format("{ML}") == "C");
    CHECK(Version(3).format("{ML}") == "D");
    CHECK(Version(24).format("{ML}") == "Y");
    CHECK(Version(25).format("{ML}") == "Z");
    CHECK(Version(26).format("{ML}") == "AA");
    CHECK(Version(27).format("{ML}") == "AB");
    CHECK(Version(28).format("{ML}") == "AC");
    CHECK(Version(26 + 25).format("{ML}") == "AZ");
    CHECK(Version(26 + 26).format("{ML}") == "BA");
    CHECK(Version(26 + 26 + 25).format("{ML}") == "BZ");
    CHECK(Version(26 + 26 + 26).format("{ML}") == "CA");
    CHECK(Version(26 * 26 - 2).format("{ML}") == "YY");
    CHECK(Version(26 * 26 - 1).format("{ML}") == "YZ");
    CHECK(Version(26 * 26).format("{ML}") == "ZA");
    CHECK(Version(26 * 26 + 26 - 2).format("{ML}") == "ZY");
    CHECK(Version(26 * 26 + 26 - 1).format("{ML}") == "ZZ");
    CHECK(Version(26 * 26 + 26).format("{ML}") == "AAA");
    CHECK(Version(26 * 26 * 26).format("{ML}") == "YZA");
    CHECK(Version(26 * 26 * 26 + 26 * 26 + 26 - 1).format("{ML}") == "ZZZ");
    CHECK(Version(26 * 26 * 26 + 26 * 26 + 26).format("{ML}") == "AAAA");

    {
        std::string s = "{ML}";
        Version v(27);
        v.format(s);
        CHECK(s == "AB");
    }
    {
        std::string s = "{mL}";
        Version v(0);
        v.format(s);
        CHECK(s == "A");
    }
    {
        std::string s = "{ML}";
        Version v(25);
        v.format(s);
        CHECK(s == "Z");
    }
    {
        std::string s = "{ML}";
        Version v(26);
        v.format(s);
        CHECK(s == "AA");
    }
    {
        std::string s = "{Ml}";
        Version v(1);
        v.format(s);
        CHECK(s == "b");
    }
    {
        std::string s = "{ML}";
        Version v(234236523);
        v.format(s);
        CHECK(s == "SROASB");
    }
    {
        std::string s = "{Ml}";
        Version v(234236523);
        v.format(s);
        CHECK(s == "sroasb");
    }
}

TEST_CASE("Checking version ranges", "[range]")
{
    using namespace primitives::version;

    // VersionRange ops
    {
        VersionRange vr1(" > 1 < 3 || >5 <7 ");
        VersionRange vr2(">2 <4||>4 <6");
        vr1 |= vr2;
        CHECK(vr1.toString() == ">=1.0.1 <4.0.0 || >=4.0.1 <7.0.0");
    }

    {
        VersionRange vr1("<2");
        VersionRange vr2(">3");
        vr1 |= vr2;
        CHECK(vr1.toString() == "<2.0.0 || >=3.0.1");
    }

    {
        VersionRange vr1("<2");
        VersionRange vr2(">3");
        vr1 &= vr2;
        CHECK(vr1.toString() == "");
    }

    {
        VersionRange vr1("<4");
        VersionRange vr2(">3");
        vr1 &= vr2;
        CHECK(vr1.toString() == ">=3.0.1 <4.0.0");
    }

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
        VersionRange vr("=1");
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
        CHECK(s == ">=0.0.3.0 <0.0.4.0");
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
        VersionRange vr("1 - 3 <2");
        auto s = vr.toString();
        CHECK(s == ">=1.0.0 <2.0.0");
    }

    {
        VersionRange vr("1-2");
        CHECK(vr.toString() == ">=1.0.0-2 <2.0.0");
    }

    {
        VersionRange vr("1-a");
        CHECK(vr.toString() == ">=1.0.0-a <2.0.0");
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
        CHECK(vr.toString() == ">=9.0.1");
    }

    {
        VersionRange vr(">=7 >8 >9");
        CHECK(vr.toString() == ">=9.0.1");
    }

    {
        VersionRange vr(">=7 >8 >=9");
        CHECK(vr.toString() == ">=9.0.0");
    }

    {
        VersionRange vr(">7 >8 <10");
        CHECK(vr.toString() == ">=8.0.1 <10.0.0");
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

    // from websites
    // https://docs.npmjs.com/misc/semver
    CHECK(VersionRange("^4.8.0 || ^5.7.0 || ^6.2.2 || >=8.0.0").toString() ==
        ">=4.8.0 <5.0.0 || >=5.7.0 <6.0.0 || >=6.2.2 <7.0.0 || >=8.0.0");
    CHECK(VersionRange("^4.8.0 || ^5.7.0 || ^6.2.2 || >8.0.0").toString() ==
        ">=4.8.0 <5.0.0 || >=5.7.0 <6.0.0 || >=6.2.2 <7.0.0 || >=8.0.1");
    CHECK(VersionRange("^4.8.0 || ^5.7.0.0 || ^6.2.2 || >8.0.0.0").toString() ==
        ">=4.8.0 <5.0.0 || >=5.7.0.0 <6.0.0.0 || >=6.2.2 <7.0.0 || >=8.0.0.1");
    CHECK(VersionRange("1.x || >=2.5.0 || 5.0.0 - 7.2.3").toString() ==
        ">=1.0.0 <2.0.0 || >=2.5.0");
    CHECK(VersionRange("1.x.x.X || >=2.5.0 || 5.0.0 - 7.2.3").toString() ==
        ">=1.0.0.0 <2.0.0.0 || >=2.5.0");
    CHECK(VersionRange("1.x.x.* || >=2.5.0 || 5.0.0 - 7.2.3").toString() ==
        ">=1.0.0.0 <2.0.0.0 || >=2.5.0");
    CHECK(VersionRange("42.6.7-alpha").toString() == ">=42.6.7-alpha <=42.6.7");
    CHECK(VersionRange("42.6.7.9-alpha").toString() == ">=42.6.7.9-alpha <=42.6.7.9");
    CHECK(VersionRange(">1.2.3-alpha.3").toString() == ">=1.2.4-alpha.3");
    CHECK(VersionRange("1.2.3 - 2.3.4").toString() == ">=1.2.3 <=2.3.4");
    CHECK(VersionRange("1.2 - 2.3.4").toString() == ">=1.2.0 <=2.3.4");
    CHECK(VersionRange("1.2.x - 2.3.4").toString() == ">=1.2.0 <=2.3.4");
    CHECK(VersionRange("1.2.* - 2.3.4").toString() == ">=1.2.0 <=2.3.4");
    CHECK(VersionRange("1.2.3.* - 2.3.4").toString() == ">=1.2.3.0 <2.3.5.0");
    CHECK(VersionRange("   1.2.3 - 2.3    ").toString() == ">=1.2.3 <2.4.0");
    CHECK(VersionRange("1.2.3 - 2").toString() == ">=1.2.3 <3.0.0");
    CHECK(VersionRange("*").toString() == "*");
    CHECK(VersionRange("*").toString() == "*");
    CHECK(VersionRange("*.x.X.*").toString() == "*");
    CHECK(VersionRange("1.x").toString() == ">=1.0.0 <2.0.0");
    CHECK(VersionRange("1").toString() == ">=1.0.0 <2.0.0");
    CHECK(VersionRange("1.*").toString() == ">=1.0.0 <2.0.0");
    CHECK(VersionRange("1.*.*.*").toString() == ">=1.0.0.0 <2.0.0.0");
    CHECK(VersionRange("1.x.x.x").toString() == ">=1.0.0.0 <2.0.0.0");
    CHECK(VersionRange("1.2.x").toString() == ">=1.2.0 <1.3.0");
    CHECK(VersionRange("").toString() == "*");
    CHECK(VersionRange("1").toString() == ">=1.0.0 <2.0.0");
    CHECK(VersionRange("1.2").toString() == ">=1.2.0 <1.3.0");
    CHECK(VersionRange("~1").toString() == ">=1.0.0 <2.0.0");
    CHECK(VersionRange("~1.2").toString() == ">=1.2.0 <1.3.0");
    CHECK(VersionRange("~1.2.3").toString() == ">=1.2.3 <1.3.0");
    CHECK(VersionRange("~1.2.3.4").toString() == ">=1.2.3.4 <1.3.0.0");
    CHECK(VersionRange("~0").toString() == "<1.0.0");
    CHECK(VersionRange("~0.2").toString() == ">=0.2.0 <0.3.0");
    CHECK(VersionRange("~0.2.3").toString() == ">=0.2.3 <0.3.0");
    CHECK(VersionRange("~0.2.3.4").toString() == ">=0.2.3.4 <0.3.0.0");
    CHECK(VersionRange("^1").toString() == ">=1.0.0 <2.0.0");
    CHECK(VersionRange("^1.2").toString() == ">=1.2.0 <2.0.0");
    CHECK(VersionRange("^1.2.3").toString() == ">=1.2.3 <2.0.0");
    CHECK(VersionRange("^1.2.3.4").toString() == ">=1.2.3.4 <2.0.0.0");
    CHECK(VersionRange("^0").toString() == "<1.0.0");
    CHECK(VersionRange("^0.2").toString() == ">=0.2.0 <0.3.0");
    CHECK(VersionRange("^0.2.3").toString() == ">=0.2.3 <0.3.0");
    CHECK(VersionRange("^0.2.3.4").toString() == ">=0.2.3.4 <0.3.0.0");
    CHECK(VersionRange("^0.0.3").toString() == "0.0.3");
    CHECK(VersionRange("^0.0.3.4").toString() == ">=0.0.3.4 <0.0.4.0");
    CHECK(VersionRange("^0.0.0.4").toString() == "0.0.0.4");
    CHECK(VersionRange("^1.2.3-beta.2").toString() == ">=1.2.3-beta.2 <2.0.0");
    CHECK(VersionRange("^0.0.3-beta").toString() == ">=0.0.3-beta <=0.0.3");
    CHECK(VersionRange("^0.0.3.4-beta").toString() == ">=0.0.3.4-beta <0.0.4.0");
    CHECK(VersionRange("^1.2.x").toString() == ">=1.2.0 <2.0.0");
    CHECK(VersionRange("^1.2.x.*").toString() == ">=1.2.0.0 <2.0.0.0");
    CHECK(VersionRange("^0.0.x").toString() == "<0.1.0");
    CHECK(VersionRange("^0.0").toString() == "<0.1.0");
    CHECK(VersionRange("^1.x").toString() == ">=1.0.0 <2.0.0");
    CHECK(VersionRange("^0.x").toString() == "<1.0.0");
    CHECK(VersionRange("~1.2.x").toString() == ">=1.2.0 <1.3.0");
    CHECK(VersionRange("~1.2.x.*").toString() == ">=1.2.0.0 <1.3.0.0");
    CHECK(VersionRange("~0.0.x").toString() == "<0.1.0");
    CHECK(VersionRange("~0.0").toString() == "<0.1.0");
    CHECK(VersionRange("~1.x").toString() == ">=1.0.0 <2.0.0");
    CHECK(VersionRange("~0.x").toString() == "<1.0.0");

    CHECK(VersionRange("~0.x-x").toString() == ">=0.0.1-x <1.0.0");
    CHECK(VersionRange("~0.x-X").toString() == ">=0.0.1-X <1.0.0");
    CHECK(VersionRange("~0.x-xx").toString() == ">=0.0.1-xx <1.0.0");
    CHECK(VersionRange("~0.x-XX").toString() == ">=0.0.1-XX <1.0.0");
    CHECK(VersionRange("~0.x-xxx").toString() == ">=0.0.1-xxx <1.0.0");
    CHECK(VersionRange("~0.x-XXX").toString() == ">=0.0.1-XXX <1.0.0");
    CHECK(VersionRange("~0.x-xXx").toString() == ">=0.0.1-xXx <1.0.0");
    CHECK(VersionRange("~0.x-XxX").toString() == ">=0.0.1-XxX <1.0.0");
    CHECK(VersionRange("~0.x-xax").toString() == ">=0.0.1-xax <1.0.0");
    CHECK(VersionRange("~0.x-XaX").toString() == ">=0.0.1-XaX <1.0.0");
    CHECK(VersionRange("~0.x-xxxax").toString() == ">=0.0.1-xxxax <1.0.0");
    CHECK(VersionRange("~0.x-XXXaX").toString() == ">=0.0.1-XXXaX <1.0.0");
    CHECK(VersionRange("~0.x-xXxAx").toString() == ">=0.0.1-xXxAx <1.0.0");
    CHECK(VersionRange("~0.x-XxXaX").toString() == ">=0.0.1-XxXaX <1.0.0");
    CHECK(VersionRange("~0.x-ax").toString() == ">=0.0.1-ax <1.0.0");
    CHECK(VersionRange("~0.x-aX").toString() == ">=0.0.1-aX <1.0.0");
    CHECK(VersionRange("~0.x-axa").toString() == ">=0.0.1-axa <1.0.0");
    CHECK(VersionRange("~0.x-aXa").toString() == ">=0.0.1-aXa <1.0.0");
    CHECK(VersionRange("~0.x.x.x-axa").toString() == ">=0.0.0.1-axa <1.0.0.0");
    CHECK(VersionRange("~0.x.x.x-aXa").toString() == ">=0.0.0.1-aXa <1.0.0.0");

    CHECK(VersionRange("^0.0") == VersionRange("~0.0"));

    // https://yarnpkg.com/lang/en/docs/dependency-versions/
    CHECK(VersionRange(">=2.0.0 <3.1.4").toString() == ">=2.0.0 <=3.1.3");
    CHECK(VersionRange("<3.1.4 >=2.0.0 ").toString() == ">=2.0.0 <=3.1.3");
    CHECK(VersionRange("<2.0.0 || >3.1.4").toString() == "<2.0.0 || >=3.1.5");
    CHECK(VersionRange("^0.4.2").toString() == ">=0.4.2 <0.5.0");
    CHECK(VersionRange("~2.7.1").toString() == ">=2.7.1 <2.8.0");
    CHECK(VersionRange("<2.0.0").toString() == "<2.0.0");
    CHECK(VersionRange("<=3.1.4 ").toString() == "<=3.1.4");
    CHECK(VersionRange(">0.4.2").toString() == ">=0.4.3");
    CHECK(VersionRange(">=2.7.1").toString() == ">=2.7.1");
    CHECK(VersionRange("=4.6.6").toString() == "4.6.6");
    CHECK(VersionRange("!=4.6.6").toString() == "<=4.6.5 || >=4.6.7");
    CHECK(VersionRange("!=4.6.6.8").toString() == "<=4.6.6.7 || >=4.6.6.9");
    CHECK(VersionRange("2.0.0 - 3.1.4").toString() == ">=2.0.0 <=3.1.4");
    CHECK(VersionRange("0.4 - 2").toString() == ">=0.4.0 <3.0.0");
    CHECK(VersionRange("2.x").toString() == ">=2.0.0 <3.0.0");
    CHECK(VersionRange("3.1.x").toString() == ">=3.1.0 <3.2.0");
    CHECK(VersionRange("2").toString() == ">=2.0.0 <3.0.0");
    CHECK(VersionRange("3.1").toString() == ">=3.1.0 <3.2.0");
    CHECK(VersionRange("~3.1.4").toString() == ">=3.1.4 <3.2.0");
    CHECK(VersionRange("~3.1").toString() == ">=3.1.0 <3.2.0");
    CHECK(VersionRange("~3").toString() == ">=3.0.0 <4.0.0");
    CHECK(VersionRange("^3.1.4").toString() == ">=3.1.4 <4.0.0");
    CHECK(VersionRange("^0.4.2").toString() == ">=0.4.2 <0.5.0");
    CHECK(VersionRange("^0.0.2").toString() == "0.0.2");

    CHECK(VersionRange("^0.0.x").toString() == "<0.1.0");
    CHECK(VersionRange("^0.0").toString() == "<0.1.0");
    CHECK(VersionRange("^0.x").toString() == "<1.0.0");
    CHECK(VersionRange("^0").toString() == "<1.0.0");

    CHECK(VersionRange(">= 1.2, < 3.0.0 || >= 4.2.3").toString() == ">=1.2.0 <3.0.0 || >=4.2.3");
    CHECK(VersionRange(">= 1.2&&< 3.0.0 || >= 4.2.3").toString() == ">=1.2.0 <3.0.0 || >=4.2.3");
    CHECK(VersionRange("<1.1 || >= 1.2&&< 3.0.0 || >= 4.2.3").toString() == "<1.1.0 || >=1.2.0 <3.0.0 || >=4.2.3");

    // https://doc.rust-lang.org/stable/cargo/reference/specifying-dependencies.html
    CHECK(VersionRange(">= 1.2.0").toString() == ">=1.2.0");
    CHECK(VersionRange("> 1").toString() == ">=1.0.1");
    CHECK(VersionRange(">= 1").toString() == ">=1.0.0");
    CHECK(VersionRange("< 2").toString() == "<2.0.0");
    CHECK(VersionRange("== 1.2.3").toString() == "1.2.3");
    CHECK(VersionRange("== 1.2.3.4").toString() == "1.2.3.4");

    CHECK(VersionRange("1.2.3.4.5.6.7").toString() == "1.2.3.4.5.6.7");
    CHECK(VersionRange("1.2.3.4.5.6.7-1").toString() == ">=1.2.3.4.5.6.7-1 <=1.2.3.4.5.6.7");

    // intervals
    CHECK_THROWS(VersionRange("[1.0]"));
    CHECK_THROWS(VersionRange("[1.0,,]"));
    CHECK_THROWS(VersionRange("[1.0,,1.0]"));
    CHECK_THROWS(VersionRange("[1.0,1.0,1.0]"));
    CHECK_THROWS(VersionRange("[1.0,1.0,]"));
    CHECK_THROWS(VersionRange("[,1.0,1.0,]"));
    CHECK_THROWS(VersionRange("[,1.0,1.0]"));
    CHECK_THROWS(VersionRange("[,,]"));
    CHECK_THROWS(VersionRange("[,]"));
    CHECK_THROWS(VersionRange("[]"));

    // left
    CHECK(VersionRange("[,2.0]").toString() == "<=2.0.0");
    CHECK(VersionRange("[,2.0.*]").toString() == "<=2.0.0");
    CHECK(VersionRange("[,2.0.5]").toString() == "<=2.0.5");
    CHECK(VersionRange("(,2.0]").toString() == "<=2.0.0");
    CHECK(VersionRange("(,2.0.*]").toString() == "<=2.0.0");
    CHECK(VersionRange("(,2.0.5]").toString() == "<=2.0.5");
    CHECK(VersionRange("(,2.0)").toString() == "<2.0.0");
    CHECK(VersionRange("(,2.0.*)").toString() == "<2.0.0");
    CHECK(VersionRange(" ( , 2.0.5 ) ").toString() == "<=2.0.4");

    // right
    CHECK(VersionRange("[1.0,]").toString() == ">=1.0.0");
    CHECK(VersionRange(" [ 1.0 , ) ").toString() == ">=1.0.0");
    CHECK(VersionRange("(1.0,]").toString() == ">=1.0.1");
    CHECK(VersionRange(" ( 1.0 , ) ").toString() == ">=1.0.1");
    CHECK(VersionRange("(1,]").toString() == ">=1.0.1");
    CHECK(VersionRange(" ( 1 , ) ").toString() == ">=1.0.1");
    CHECK(VersionRange("(1.x,]").toString() == ">=1.0.1");
    CHECK(VersionRange(" ( 1.* , ) ").toString() == ">=1.0.1");
    CHECK(VersionRange("(1.5.*,]").toString() == ">=1.5.1");
    CHECK(VersionRange(" ( 1.5.X , ) ").toString() == ">=1.5.1");

    // both
    CHECK(VersionRange("[1,2]").toString() == ">=1.0.0 <=2.0.0");
    CHECK(VersionRange("[1,2)").toString() == ">=1.0.0 <2.0.0");
    CHECK(VersionRange("(1,2]").toString() == ">=1.0.1 <=2.0.0");
    CHECK(VersionRange("(1,2)").toString() == ">=1.0.1 <2.0.0");
    CHECK(VersionRange("(1.2.3,2)").toString() == ">=1.2.4 <2.0.0");
    CHECK(VersionRange("(1.2.3.4,2.0.0.0)").toString() == ">=1.2.3.5 <2.0.0.0");
    CHECK(VersionRange("(1.2.3.4,2)").toString() == ">=1.2.3.5 <2.0.0.0");
    CHECK(VersionRange("(1,2.3.4.5)").toString() == ">=1.0.0.1 <=2.3.4.4");
    CHECK(VersionRange("(1,2.3.4.5]").toString() == ">=1.0.0.1 <=2.3.4.5");
    CHECK(VersionRange("(1,2.3.4.5.6.7.8.9.10.11.12.13.14.15.16.17]").toString()
        == ">=1.0.0.0.0.0.0.0.0.0.0.0.0.0.0.1 <=2.3.4.5.6.7.8.9.10.11.12.13.14.15.16.17");

    // complex
    CHECK(VersionRange("[1,4] [2,3]").toString() == ">=2.0.0 <=3.0.0");
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
