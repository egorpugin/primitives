// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

//#include <primitives/sw/main.h>
#include <primitives/version_range.h>

#include <chrono>
#include <iostream>

//#define CATCH_CONFIG_RUNNER
#include <catch2/catch_all.hpp>
/*namespace Catch {
    CATCH_REGISTER_REPORTER("console", ConsoleReporter)
}*/

TEST_CASE("Checking package version and ranges", "[package]")
{
    using namespace primitives::version;

    // valid branches
    /*CHECK_NOTHROW(PackageVersion("a"));
    CHECK_NOTHROW(PackageVersion("ab"));
    CHECK_NOTHROW(PackageVersion("abc123___"));
    // too long branch
    CHECK_THROWS(PackageVersion("abc123___abc123___abc123___abc123___abc123___abc123___abc123___abc123___abc123___abc123___abc123___abc123___abc123___abc123___abc123___abc123___abc123___abc123___abc123___abc123___abc123___abc123___abc123___abc123___abc123___"));
    CHECK_NOTHROW(PackageVersion("_1abc123___"));
    CHECK_THROWS(PackageVersion("1abc123___"));
    CHECK_THROWS(PackageVersion("abc123___-")); // no extra allowed
    CHECK_THROWS(PackageVersion("a-a")); // extras on branches are not allowed

    {
        PackageVersion pv, pv1;
        Version &v = pv.getVersion();
        Version &v1 = pv1.getVersion();
        v = "0.0.1";
        CHECK(v.getMajor() == 0);
        CHECK(v.getMinor() == 0);
        CHECK(v.getPatch() == 1);
        CHECK(v.getTweak() == 0);
        /*v.incrementVersion();
        CHECK(v.getPatch() == 2);
        CHECK(v.getTweak() == 0);
        v.incrementVersion();
        CHECK(v.getPatch() == 3);
        CHECK(v.getTweak() == 0);
        v.decrementVersion();
        v.decrementVersion();
        CHECK(v.getPatch() == 1);
        CHECK(v.getTweak() == 0);
        v.decrementVersion();
        CHECK(v.getPatch() == 1);
        CHECK(v.getTweak() == 0);
        v.decrementVersion();
        v.decrementVersion();
        CHECK(v.getPatch() == 1);
        CHECK(v.getTweak() == 0);
        CHECK(pv.isVersion());
        CHECK_FALSE(pv.isBranch());
        v = v1;
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
        CHECK(pv.isVersion());
        CHECK_FALSE(pv.isBranch());
        v = "1.2.3.4";
        CHECK(v.getMajor() == 1);
        CHECK(v.getMinor() == 2);
        CHECK(v.getPatch() == 3);
        CHECK(v.getTweak() == 4);
        v.incrementVersion();
        CHECK(v.getTweak() == 5);
        v.incrementVersion();
        CHECK(v.getTweak() == 6);
        v.decrementVersion();
        v.decrementVersion();
        CHECK(v.getTweak() == 4);
        v.decrementVersion();
        CHECK(v.getTweak() == 3);
        v.decrementVersion();
        v.decrementVersion();
        CHECK(v.getTweak() == 1);
        CHECK(pv.isVersion());
        CHECK_FALSE(pv.isBranch());/
    }

    {
        PackageVersion pv;
        Version &v = pv.getVersion();
        CHECK(v.getMajor() == 0);
        CHECK(v.getMinor() == 0);
        CHECK(v.getPatch() == 1);
        CHECK(v.getTweak() == 0);
        /*v.incrementVersion();
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
        CHECK(pv.isVersion());
        CHECK_FALSE(pv.isBranch());/
    }

    {
        Version v("0");
        CHECK(v.getMajor() == 0);
        CHECK(v.getMinor() == 0);
        CHECK(v.getPatch() == 0);
        CHECK(v.getTweak() == 0);
        /*v.decrementVersion();
        v.decrementVersion();
        v.decrementVersion();
        CHECK(v.getMajor() == 0);
        CHECK(v.getMinor() == 0);
        CHECK(v.getPatch() == 0);
        CHECK(v.getTweak() == 0);/
    }

    {
        PackageVersion v1("0");
        auto &v = v1.getVersion();
        CHECK(v.getMajor() == 0);
        CHECK(v.getMinor() == 0);
        CHECK(v.getPatch() == 1);
        CHECK(v.getTweak() == 0);
        /*v.decrementVersion();
        v.decrementVersion();
        v.decrementVersion();
        CHECK(v.getMajor() == 0);
        CHECK(v.getMinor() == 0);
        CHECK(v.getPatch() == 1);
        CHECK(v.getTweak() == 0);/
    }

    {
        PackageVersion v1("0.0.0");
        auto &v = v1.getVersion();
        CHECK(v.getMajor() == 0);
        CHECK(v.getMinor() == 0);
        CHECK(v.getPatch() == 1);
        CHECK(v.getTweak() == 0);
        /*v.decrementVersion();
        v.decrementVersion();
        v.decrementVersion();
        CHECK(v.getMajor() == 0);
        CHECK(v.getMinor() == 0);
        CHECK(v.getPatch() == 1);
        CHECK(v.getTweak() == 0);/
    }

    {
        Version v = Version::min();
        CHECK(v.getMajor() == 0);
        CHECK(v.getMinor() == 0);
        CHECK(v.getPatch() == 0);
        CHECK(v.getTweak() == 0);
    }

    {
        PackageVersion pv;
        Version &v = pv.getVersion();
        CHECK(v.getMajor() == 0);
        CHECK(v.getMinor() == 0);
        CHECK(v.getPatch() == 1);
        CHECK(v.getTweak() == 0);
        /*v.decrementVersion();
        v.decrementVersion();
        v.decrementVersion();
        CHECK(v.getMajor() == 0);
        CHECK(v.getMinor() == 0);
        CHECK(v.getPatch() == 1);
        CHECK(v.getTweak() == 0);/
    }

    {
        /*Version v1(0, 0, 0);
        Version v = Version::min(v1.getLevel());
        CHECK(v.getMajor() == 0);
        CHECK(v.getMinor() == 0);
        CHECK(v.getPatch() == 1);
        CHECK(v.getTweak() == 0);*/
        /*v.decrementVersion();
        v.decrementVersion();
        v.decrementVersion();
        CHECK(v.getMajor() == 0);
        CHECK(v.getMinor() == 0);
        CHECK(v.getPatch() == 1);
        CHECK(v.getTweak() == 0);/
    }

    {
        //Version v1("branch");
        //CHECK_THROWS(v1.getLevel());
    }

    {
        Version v = Version::max();
        CHECK(v.getMajor() == Version::maxNumber() + 1);
        CHECK(v.getMinor() == 0);
        CHECK(v.getPatch() == 0);
        CHECK(v.getTweak() == 0);
    }

    {
        /*Version v = Version::max(Version::getDefaultLevel());
        CHECK(v.getMajor() == Version::maxNumber());
        CHECK(v.getMinor() == Version::maxNumber());
        CHECK(v.getPatch() == Version::maxNumber());
        CHECK(v.getTweak() == 0);*/
        /*v.decrementVersion();
        v.decrementVersion();
        v.decrementVersion();
        CHECK(v.getMajor() == Version::maxNumber());
        CHECK(v.getMinor() == Version::maxNumber());
        CHECK(v.getPatch() == Version::maxNumber() - 3);
        CHECK(v.getTweak() == 0);/
    }

    {
        /*Version v1(0, 0, 0);
        Version v = Version::max(v1.getLevel());
        CHECK(v.getMajor() == Version::maxNumber());
        CHECK(v.getMinor() == Version::maxNumber());
        CHECK(v.getPatch() == Version::maxNumber());
        CHECK(v.getTweak() == 0);*/
        /*v.decrementVersion();
        v.decrementVersion();
        v.decrementVersion();
        CHECK(v.getMajor() == Version::maxNumber());
        CHECK(v.getMinor() == Version::maxNumber());
        CHECK(v.getPatch() == Version::maxNumber() - 3);
        CHECK(v.getTweak() == 0);/
    }

    {
        PackageVersion v1("00000000000");
        auto &v = v1.getVersion();
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
        CHECK(v.getPatch() == 0);
        CHECK(v.getTweak() == 0);
    }

    {
        PackageVersion v1("0.0");
        auto &v = v1.getVersion();
        CHECK(v.getMajor() == 0);
        CHECK(v.getMinor() == 0);
        CHECK(v.getPatch() == 1);
        CHECK(v.getTweak() == 0);
    }

    {
        Version v(0,0);
        CHECK(v.getMajor() == 0);
        CHECK(v.getMinor() == 0);
        CHECK(v.getPatch() == 0);
        CHECK(v.getTweak() == 0);
    }

    {
        PackageVersion v1("0.0.0");
        auto &v = v1.getVersion();
        CHECK(v.getMajor() == 0);
        CHECK(v.getMinor() == 0);
        CHECK(v.getPatch() == 1);
        CHECK(v.getTweak() == 0);
        /*v.incrementVersion();
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
        CHECK(v.getPatch() == 1);/
    }

    {
        Version v(0, "rc2.3._a_");
        CHECK(v.getMajor() == 0);
        CHECK(v.getMinor() == 0);
        CHECK(v.getPatch() == 0);
        CHECK(v.getTweak() == 0);
        CHECK(v.getExtra()[0] == "rc2");
        CHECK(v.getExtra()[1] == 3);
        CHECK(v.getExtra()[2] == "_a_");
        PackageVersion v1(v);
        CHECK(v1.getVersion().getPatch() == 1);
    }

    {
        Version v(Version(0,0,0), "rc2.3._a_");
        CHECK(v.getMajor() == 0);
        CHECK(v.getMinor() == 0);
        CHECK(v.getPatch() == 0);
        CHECK(v.getTweak() == 0);
        CHECK(v.getExtra()[0] == "rc2");
        CHECK(v.getExtra()[1] == 3);
        CHECK(v.getExtra()[2] == "_a_");
        PackageVersion v1(v);
        CHECK(v1.getVersion().getPatch() == 1);
    }


    {
        CHECK(PackageVersion().format("{v}") == "0.0.1");
        CHECK_THROWS(PackageVersion().format("{b}") == "");
        CHECK(PackageVersion().format("{e}") == "");
        CHECK(Version("1-e").format("{e}") == "e");
        CHECK(Version("1-e.2.3.4").format("{e}") == "e.2.3.4");
        CHECK(Version(Version(1), "e.2.3.4").format("{e}") == "e.2.3.4");
        CHECK(PackageVersion("branch").format("{v}") == "branch");
        CHECK(PackageVersion("branch").format("{b}") == "branch");

        CHECK(PackageVersion().format("{M}") == "0");
        CHECK(PackageVersion().format("{m}") == "0");
        CHECK(PackageVersion().format("{p}") == "1");
        CHECK(PackageVersion().format("{t}") == "0");
        //CHECK(PackageVersion().format("{5}") == "0");
        CHECK(PackageVersion().format("{M}{m}{p}") == "001");
        CHECK(PackageVersion().format("{M}{m}{po}") == "00.1");
        CHECK(Version(1).format("{M}{mo}{po}") == "1");
        CHECK(Version(1,2).format("{M}{mo}{po}") == "1.2");
        CHECK(Version(1,2,3).format("{M}{mo}{po}") == "1.2.3");

        CHECK(PackageVersion().format("{ML}") == "A");
        CHECK(PackageVersion().format("{mL}") == "A");
        CHECK(PackageVersion().format("{pL}") == "B");
        CHECK(PackageVersion().format("{tL}") == "A");
        //CHECK(Version().format("{5L}") == "A");
        CHECK(PackageVersion().format("{ML}{mL}{pL}") == "AAB");
        CHECK(PackageVersion().format("{ML}{mL}{pLo}") == "AA.B");
        CHECK(Version(1).format("{ML}{mLo}{pLo}") == "B");
        CHECK(Version(1, 2).format("{ML}{mLo}{pLo}") == "B.C");
        CHECK(Version(1, 2, 3).format("{ML}{mLo}{pLo}") == "B.C.D");

        CHECK(PackageVersion().format("{Ml}") == "a");
        CHECK(PackageVersion().format("{ml}") == "a");
        CHECK(PackageVersion().format("{pl}") == "b");
        CHECK(PackageVersion().format("{tl}") == "a");
        //CHECK(PackageVersion().format("{5l}") == "a");
        CHECK(PackageVersion().format("{Ml}{ml}{pl}") == "aab");
        CHECK(PackageVersion().format("{Ml}{ml}{plo}") == "aa.b");
        CHECK(Version(1).format("{Ml}{mlo}{plo}") == "b");
        CHECK(Version(1, 2).format("{Ml}{mlo}{plo}") == "b.c");
        CHECK(Version(1, 2, 3).format("{Ml}{mlo}{plo}") == "b.c.d");
    }

    // cmp
    {
        CHECK(PackageVersion("a") == PackageVersion("a"));
        CHECK_FALSE(PackageVersion("a") != PackageVersion("a"));
        CHECK(PackageVersion("a") < PackageVersion());
        CHECK(PackageVersion("a") <= PackageVersion());
        CHECK(PackageVersion() > PackageVersion("a"));
        CHECK(PackageVersion() >= PackageVersion("a"));
        CHECK(PackageVersion() != PackageVersion("a"));
        CHECK(PackageVersion("a") < PackageVersion("b"));
        CHECK(PackageVersion("1") > PackageVersion("b"));

        CHECK(Version(Version(2, 14, 0), "rc16") != Version(2, 14, 0));
        CHECK(Version(Version(2, 14, 0), "rc16") < Version(2, 14, 0));
        CHECK(Version(Version(2, 14, 0), "rc16") <= Version(2, 14, 0));
        CHECK_FALSE(Version(Version(2, 14, 0), "rc16") > Version(2, 14, 0));
        CHECK_FALSE(Version(Version(2, 14, 0), "rc16") >= Version(2, 14, 0));

        VersionSet a;
        a.insert("master");
        a.insert("develop");
        a.insert("57");
        a.insert("58");
        a.insert("59");

        CHECK(*a.begin() == "develop");
        CHECK(*std::next(a.begin()) == "master");
        CHECK(*std::prev(a.end()) == Version(59));

        VersionSetCustom<std::greater<PackageVersion>> b(a.begin(), a.end());

        CHECK(*b.begin() == "59");
        CHECK(*std::next(b.begin()) == "58");
        CHECK(*std::prev(b.end()) == PackageVersion("develop"));
    }

    // ==
    {
        CHECK(Version() == Version());
        CHECK(PackageVersion() == PackageVersion());
        CHECK(PackageVersion() == Version(0));
        CHECK(PackageVersion(Version()) == Version(0));
        CHECK(PackageVersion(Version()) == PackageVersion(Version(0)));
        CHECK(PackageVersion() == Version(0, 0));
        CHECK(PackageVersion() == Version(0, 0, 0));
        CHECK(PackageVersion() == Version(0, 0, 1));
        CHECK_FALSE(Version() != Version());
        CHECK(Version(0) == Version(0));
        CHECK(Version(0) == Version(0, 0));
        CHECK(Version(0) == Version(0, 0, 0));
        CHECK(PackageVersion(Version(0)) == Version(0, 0, 1));
        CHECK(Version(0) == Version(0, 0, 0, 0));
        CHECK(PackageVersion(Version(0)) != Version(0, 0, 0, 0)); // 0.0.1 != 0.0.0.1
        CHECK(Version(0) == Version({ 0, 0, 0, 0, 0 }));
        CHECK(PackageVersion(Version(0)) != Version({ 0, 0, 0, 0, 0 })); // 0.0.1 != 0.0.0.0.1
        CHECK(Version(0) != Version({ 0, 0, 1, 0, 0 }));
        CHECK(PackageVersion(Version(0)) == Version({ 0, 0, 1, 0, 0 }));
        CHECK(Version(1) == Version(1));
        CHECK(Version(1) == Version(1, 0));
        CHECK(Version(1) == Version(1, 0, 0));
        CHECK(Version(1) == Version(1, 0, 0, 0));
        CHECK(Version(1) == Version({ 1, 0, 0, 0, 0 }));
        CHECK(Version(1, 2) == Version(1, 2));
        CHECK(Version(1, 2) == Version(1, 2, 0));
        CHECK(Version(1, 2) == Version(1, 2, 0, 0));
        CHECK(Version(1, 2, 3) == Version(1, 2, 3, 0));
        CHECK(Version(1, 2, 3) == Version({ 1, 2, 3, 0, 0 }));
        CHECK(Version(1, 2, 3, 4) == Version({ 1, 2, 3, 4, 0 }));
        CHECK(Version(1, 2, 3, 4) == Version({1, 2, 3, 4, 0, 0}));
        CHECK(Version(1, 1, 1) == Version({1, 1, 1, 0}));
        CHECK(Version(1, 1, 1) == Version({1, 1, 1, 0, 0}));
    }

    // toString()
    {
        PackageVersion v1;
        auto &v = v1.getVersion();
        CHECK(v.toString() == "0.0.1");
        CHECK(v.toString(5) == "0.0.1.0.0");
        CHECK(v.toString(4) == "0.0.1.0");
        CHECK(v.toString(3) == "0.0.1");
        CHECK(v.toString(2) == "0.0.1");
        CHECK(v.toString(1) == "0.0.1");
        CHECK(Version().toString(0) == "");
        CHECK(Version().toString(-1) == "");
        CHECK(Version().toString(-2) == "");
        CHECK(Version().toString(-3) == "");
    }

    */
}

TEST_CASE("Checking versions", "[version]")
{
    using namespace primitives::version;

    CHECK_THROWS(Version("1..1"));
    CHECK_THROWS(Version("1.1-2..2"));
    CHECK_THROWS(Version("1.1-2-2"));
    CHECK_THROWS(Version("1.1-")); // empty extra
    CHECK_THROWS(Version(""));
    CHECK_THROWS(Version("-"));
    CHECK_THROWS(Version("."));

    CHECK_NOTHROW(Version("1"));
    CHECK_NOTHROW(Version("1-alpha1")); // with extra
    CHECK_NOTHROW(Version("1.2"));
    CHECK_NOTHROW(Version("1.2-rc2.3.a")); // with extra
    CHECK_THROWS(Version("1.2--rc2.3.a")); // with extra
    CHECK_THROWS(Version("1.2-rc2.3.a-")); // with extra
    CHECK_THROWS(Version("1.2-rc2.3.-a")); // with extra
    CHECK_THROWS(Version(nullptr));
    CHECK_THROWS(Version(Version("1.2"), "-rc2.3.-a")); // with extra
    CHECK_THROWS(Version(Version("1.2"), "3.4.1-beta4-19610-02")); // with extra
    CHECK_THROWS(Version(Version("1.2"), "beta4-19610-02")); // with extra
    CHECK_NOTHROW(Version("1.2.3"));
    CHECK_NOTHROW(Version("1.2.3.4"));
    CHECK_THROWS(Version("1.2.*"));
    CHECK_THROWS(Version("1.2.x"));
    CHECK_THROWS(Version("1.2.X"));
    CHECK_THROWS(Version(Version(0), "-rc2.3._a_"));
    CHECK_NOTHROW(Version(-1));
    CHECK_NOTHROW(Version(-1).getMajor() == -1);
    CHECK_NOTHROW(Version(-1, "rc2.3._a_"));
    CHECK_THROWS(Version(-1, "-rc2.3._a_"));

    // we allowed now versions with numbers > 4
    CHECK_NOTHROW(Version("1.2.3.4.5"));
    CHECK_NOTHROW(Version("1.2.3.4.5.6"));
    CHECK_NOTHROW(Version("1.2.3.4.5.6.7"));

    {
        CHECK_NOTHROW(Version(-1));
        CHECK_NOTHROW(Version(-1,-1));
        CHECK_NOTHROW(Version(1,-1));
        CHECK_NOTHROW(Version(-1,-1,-1));
        CHECK_NOTHROW(Version(1,1,-1));
        CHECK_NOTHROW(Version(-1,-1,-1,-1));
        CHECK_NOTHROW(Version(1,1,1,-1));
        CHECK_NOTHROW(Version({ -1,-1,-1,-1,-1 }));
    }

    {
        Version v;
        v = "0.0.1";
        CHECK(v.getMajor() == 0);
        CHECK(v.getMinor() == 0);
        CHECK(v.getPatch() == 1);
        CHECK(v.getTweak() == 0);
        /*v.incrementVersion();
        CHECK(v.getPatch() == 2);
        CHECK(v.getTweak() == 0);
        v.incrementVersion();
        CHECK(v.getPatch() == 3);
        CHECK(v.getTweak() == 0);
        v.decrementVersion();
        v.decrementVersion();
        CHECK(v.getPatch() == 1);
        CHECK(v.getTweak() == 0);
        v.decrementVersion();
        CHECK(v.getPatch() == 1);
        CHECK(v.getTweak() == 0);
        v.decrementVersion();
        v.decrementVersion();
        CHECK(v.getPatch() == 1);
        CHECK(v.getTweak() == 0);
        CHECK(pv.isVersion());
        CHECK_FALSE(pv.isBranch());
        v = v1;
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
        CHECK(pv.isVersion());
        CHECK_FALSE(pv.isBranch());
        v = "1.2.3.4";
        CHECK(v.getMajor() == 1);
        CHECK(v.getMinor() == 2);
        CHECK(v.getPatch() == 3);
        CHECK(v.getTweak() == 4);
        v.incrementVersion();
        CHECK(v.getTweak() == 5);
        v.incrementVersion();
        CHECK(v.getTweak() == 6);
        v.decrementVersion();
        v.decrementVersion();
        CHECK(v.getTweak() == 4);
        v.decrementVersion();
        CHECK(v.getTweak() == 3);
        v.decrementVersion();
        v.decrementVersion();
        CHECK(v.getTweak() == 1);
        CHECK(pv.isVersion());
        CHECK_FALSE(pv.isBranch());*/
    }

    {
        Version v;
        v = "0.0.1";
        CHECK(v.getMajor() == 0);
        CHECK(v.getMinor() == 0);
        CHECK(v.getPatch() == 1);
        CHECK(v.getTweak() == 0);
        /*v.incrementVersion();
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
        CHECK(pv.isVersion());
        CHECK_FALSE(pv.isBranch());*/
    }

    {
        Version v("0");
        CHECK(v.getMajor() == 0);
        CHECK(v.getMinor() == 0);
        CHECK(v.getPatch() == 0);
        CHECK(v.getTweak() == 0);
        /*v.decrementVersion();
        v.decrementVersion();
        v.decrementVersion();
        CHECK(v.getMajor() == 0);
        CHECK(v.getMinor() == 0);
        CHECK(v.getPatch() == 0);
        CHECK(v.getTweak() == 0);*/
    }

    {
        Version v;
        v = "0.0.1";
        CHECK(v.getMajor() == 0);
        CHECK(v.getMinor() == 0);
        CHECK(v.getPatch() == 1);
        CHECK(v.getTweak() == 0);
        /*v.decrementVersion();
        v.decrementVersion();
        v.decrementVersion();
        CHECK(v.getMajor() == 0);
        CHECK(v.getMinor() == 0);
        CHECK(v.getPatch() == 1);
        CHECK(v.getTweak() == 0);*/
    }

    {
        Version v;
        v = "0.0.1";
        CHECK(v.getMajor() == 0);
        CHECK(v.getMinor() == 0);
        CHECK(v.getPatch() == 1);
        CHECK(v.getTweak() == 0);
        /*v.decrementVersion();
        v.decrementVersion();
        v.decrementVersion();
        CHECK(v.getMajor() == 0);
        CHECK(v.getMinor() == 0);
        CHECK(v.getPatch() == 1);
        CHECK(v.getTweak() == 0);*/
    }

    {
        Version v = Version::min();
        CHECK(v.getMajor() == 0);
        CHECK(v.getMinor() == 0);
        CHECK(v.getPatch() == 0);
        CHECK(v.getTweak() == 0);
    }

    {
        Version v;
        v = "0.0.1";
        CHECK(v.getMajor() == 0);
        CHECK(v.getMinor() == 0);
        CHECK(v.getPatch() == 1);
        CHECK(v.getTweak() == 0);
        /*v.decrementVersion();
        v.decrementVersion();
        v.decrementVersion();
        CHECK(v.getMajor() == 0);
        CHECK(v.getMinor() == 0);
        CHECK(v.getPatch() == 1);
        CHECK(v.getTweak() == 0);*/
    }

    {
        /*Version v1(0, 0, 0);
        Version v = Version::min(v1.getLevel());
        CHECK(v.getMajor() == 0);
        CHECK(v.getMinor() == 0);
        CHECK(v.getPatch() == 1);
        CHECK(v.getTweak() == 0);*/
        /*v.decrementVersion();
        v.decrementVersion();
        v.decrementVersion();
        CHECK(v.getMajor() == 0);
        CHECK(v.getMinor() == 0);
        CHECK(v.getPatch() == 1);
        CHECK(v.getTweak() == 0);*/
    }

    {
        //Version v1("branch");
        //CHECK_THROWS(v1.getLevel());
    }

    {
        Version v = Version::max();
        CHECK(v.getMajor() == Version::maxNumber() + 1);
        CHECK(v.getMinor() == 0);
        CHECK(v.getPatch() == 0);
        CHECK(v.getTweak() == 0);
    }

    {
        /*Version v = Version::max(Version::getDefaultLevel());
        CHECK(v.getMajor() == Version::maxNumber());
        CHECK(v.getMinor() == Version::maxNumber());
        CHECK(v.getPatch() == Version::maxNumber());
        CHECK(v.getTweak() == 0);*/
        /*v.decrementVersion();
        v.decrementVersion();
        v.decrementVersion();
        CHECK(v.getMajor() == Version::maxNumber());
        CHECK(v.getMinor() == Version::maxNumber());
        CHECK(v.getPatch() == Version::maxNumber() - 3);
        CHECK(v.getTweak() == 0);*/
    }

    {
        /*Version v1(0, 0, 0);
        Version v = Version::max(v1.getLevel());
        CHECK(v.getMajor() == Version::maxNumber());
        CHECK(v.getMinor() == Version::maxNumber());
        CHECK(v.getPatch() == Version::maxNumber());
        CHECK(v.getTweak() == 0);*/
        /*v.decrementVersion();
        v.decrementVersion();
        v.decrementVersion();
        CHECK(v.getMajor() == Version::maxNumber());
        CHECK(v.getMinor() == Version::maxNumber());
        CHECK(v.getPatch() == Version::maxNumber() - 3);
        CHECK(v.getTweak() == 0);*/
    }

    {
        Version v;
        v = "0.0.1";
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
        CHECK(v.getPatch() == 0);
        CHECK(v.getTweak() == 0);
    }

    {
        Version v;
        v = "0.0.1";
        CHECK(v.getMajor() == 0);
        CHECK(v.getMinor() == 0);
        CHECK(v.getPatch() == 1);
        CHECK(v.getTweak() == 0);
    }

    {
        Version v(0,0);
        CHECK(v.getMajor() == 0);
        CHECK(v.getMinor() == 0);
        CHECK(v.getPatch() == 0);
        CHECK(v.getTweak() == 0);
    }

    {
        Version v;
        v = "0.0.1";
        CHECK(v.getMajor() == 0);
        CHECK(v.getMinor() == 0);
        CHECK(v.getPatch() == 1);
        CHECK(v.getTweak() == 0);
        /*v.incrementVersion();
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
        CHECK(v.getPatch() == 1);*/
    }

    {
        Version v(0, 0, 0);
        CHECK(v.getMajor() == 0);
        CHECK(v.getMinor() == 0);
        CHECK(v.getPatch() == 0);
        CHECK(v.getTweak() == 0);
    }

    {
        Version v("1");
        CHECK(v.getMajor() == 1);
        CHECK(v.getMinor() == 0);
        CHECK(v.getPatch() == 0);
        CHECK(v.getTweak() == 0);
        /*v.incrementVersion();
        CHECK(v.getPatch() == 1);
        v.decrementVersion();
        CHECK(v.getPatch() == 0);
        v.decrementVersion();
        CHECK(v.getMinor() == v.maxNumber());
        CHECK(v.getPatch() == v.maxNumber());
        v.decrementVersion();
        CHECK(v.getPatch() == v.maxNumber() - 1);*/
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
        /*v.decrementVersion();
        CHECK(v.getMinor() == 1);
        CHECK(v.getPatch() == v.maxNumber());*/
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
        //Version v("a");
        //CHECK(v.getBranch() == "a");
        //CHECK(v.isBranch());
        //CHECK_FALSE(v.isVersion());
    }

    {
        Version v(0, "rc2.3._a_");
        CHECK(v.getMajor() == 0);
        CHECK(v.getMinor() == 0);
        CHECK(v.getPatch() == 0);
        CHECK(v.getTweak() == 0);
        CHECK(v.getExtra()[0] == "rc2");
        CHECK(v.getExtra()[1] == 3);
        CHECK(v.getExtra()[2] == "_a_");
    }

    {
        Version v(Version(0,0,0), "rc2.3._a_");
        CHECK(v.getMajor() == 0);
        CHECK(v.getMinor() == 0);
        CHECK(v.getPatch() == 0);
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
        /*v.incrementVersion(2);
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
        CHECK(v.getTweak() == Version::maxNumber());*/
    }

    {
        Version v(1, 2, 0);
        CHECK(v.getMajor() == 1);
        CHECK(v.getMinor() == 2);
        CHECK(v.getPatch() == 0);
        CHECK(v.getTweak() == 0);
        /*v.incrementVersion(2);
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
        CHECK(v.getTweak() == Version::maxNumber());*/
    }

    // ++, --
    {
        Version v(1, 2);
        CHECK(v.getMajor() == 1);
        CHECK(v.getMinor() == 2);
        CHECK(v.getPatch() == 0);
        auto v_old = v++;
        CHECK(v.getMajor() == 1);
        CHECK(v.getMinor() == 3);
        CHECK(v.getPatch() == 0);
        CHECK(v_old.getMajor() == 1);
        CHECK(v_old.getMinor() == 2);
        CHECK(v_old.getPatch() == 0);
        v_old = v--;
        CHECK(v.getMajor() == 1);
        CHECK(v.getMinor() == 2);
        CHECK(v.getPatch() == 0);
        CHECK(v_old.getMajor() == 1);
        CHECK(v_old.getMinor() == 3);
        CHECK(v_old.getPatch() == 0);
        v_old = ++v;
        CHECK(v.getMajor() == 1);
        CHECK(v.getMinor() == 3);
        CHECK(v.getPatch() == 0);
        CHECK(v_old.getMajor() == 1);
        CHECK(v_old.getMinor() == 3);
        CHECK(v_old.getPatch() == 0);
        v_old = --v;
        CHECK(v.getMajor() == 1);
        CHECK(v.getMinor() == 2);
        CHECK(v.getPatch() == 0);
        CHECK(v_old.getMajor() == 1);
        CHECK(v_old.getMinor() == 2);
        CHECK(v_old.getPatch() == 0);
    }

    CHECK(Version(Version(0, 0, 0), "rc2.3._a_") > Version(Version(0, 0, 0), "rc1.3._a_"));
    CHECK(Version(Version(0, 0, 0), "rc2.3._a_") > Version(Version(0, 0, 0), "beta.3._a_"));
    CHECK(Version(Version(0, 0, 0), "rc2.3._a_") > Version(Version(0, 0, 0), "alpha.3._a_"));
    CHECK(Version(Version(0, 0, 0), "rc2.3._a_") < Version(Version(0, 0, 0), "rc3.3._a_"));
    CHECK(Version(Version(0, 0, 0), "rc.2.3._a_") < Version(Version(0, 0, 0), "rc.3.3._a_"));
    CHECK(Version(Version(0, 0, 0), "rc.2.3._a_") == Version(Version(0, 0, 0), "rc.2.3._a_"));

    CHECK(Version(Version(0, 0, 0), "1") == Version(Version(0, 0, 0), "1"));
    CHECK(Version(Version(0, 0, 0), "1") < Version(Version(0, 0, 0), "2"));
    CHECK(Version(Version(0, 0, 0), "1.1") < Version(Version(0, 0, 0), "2.1"));
    CHECK(Version(Version(0, 0, 0), "1.1") < Version(Version(0, 0, 0), "1.2"));
    CHECK(Version(Version(0, 0, 0), "1.1") == Version(Version(0, 0, 0), "1.1"));
    CHECK(Version(Version(0, 0, 0), "1.1.a") < Version(Version(0, 0, 0), "1.1.z"));

    // format
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
        CHECK(v.format(s) == "AB");
    }
    {
        std::string s = "{mL}";
        Version v(0);
        CHECK(v.format(s) == "A");
    }
    {
        std::string s = "{ML}";
        Version v(25);
        CHECK(v.format(s) == "Z");
    }
    {
        std::string s = "{ML}";
        Version v(26);
        CHECK(v.format(s) == "AA");
    }
    {
        std::string s = "{Ml}";
        Version v(1);
        CHECK(v.format(s) == "b");
    }
    {
        std::string s = "{ML}";
        Version v(234236523);
        CHECK(v.format(s) == "SROASB");
    }
    {
        std::string s = "{Ml}";
        Version v(234236523);
        CHECK(v.format(s) == "sroasb");
    }

    {
        CHECK(Version("1-e").format("{e}") == "e");
        CHECK(Version("1-e.2.3.4").format("{e}") == "e.2.3.4");
        CHECK(Version(Version(1), "e.2.3.4").format("{e}") == "e.2.3.4");

        CHECK(Version(1).format("{M}{mo}{po}") == "1");
        CHECK(Version(1,2).format("{M}{mo}{po}") == "1.2");
        CHECK(Version(1,2,3).format("{M}{mo}{po}") == "1.2.3");

        CHECK(Version(1).format("{ML}{mLo}{pLo}") == "B");
        CHECK(Version(1, 2).format("{ML}{mLo}{pLo}") == "B.C");
        CHECK(Version(1, 2, 3).format("{ML}{mLo}{pLo}") == "B.C.D");

        CHECK(Version(1).format("{Ml}{mlo}{plo}") == "b");
        CHECK(Version(1, 2).format("{Ml}{mlo}{plo}") == "b.c");
        CHECK(Version(1, 2, 3).format("{Ml}{mlo}{plo}") == "b.c.d");
    }

    // cmp
    {
        CHECK(Version(Version(2, 14, 0), "rc16") != Version(2, 14, 0));
        CHECK(Version(Version(2, 14, 0), "rc16") < Version(2, 14, 0));
        CHECK(Version(Version(2, 14, 0), "rc16") <= Version(2, 14, 0));
        CHECK_FALSE(Version(Version(2, 14, 0), "rc16") > Version(2, 14, 0));
        CHECK_FALSE(Version(Version(2, 14, 0), "rc16") >= Version(2, 14, 0));
    }

    // ==
    {
        CHECK(Version() == Version());
        CHECK_FALSE(Version() != Version());
        CHECK(Version(0) == Version(0));
        CHECK(Version(0) == Version(0, 0));
        CHECK(Version(0) == Version(0, 0, 0));
        CHECK(Version(0) == Version(0, 0, 0, 0));
        CHECK(Version(0) == Version({ 0, 0, 0, 0, 0 }));
        CHECK(Version(0) != Version({ 0, 0, 1, 0, 0 }));
        CHECK(Version(1) == Version(1));
        CHECK(Version(1) == Version(1, 0));
        CHECK(Version(1) == Version(1, 0, 0));
        CHECK(Version(1) == Version(1, 0, 0, 0));
        CHECK(Version(1) == Version({ 1, 0, 0, 0, 0 }));
        CHECK(Version(1, 2) == Version(1, 2));
        CHECK(Version(1, 2) == Version(1, 2, 0));
        CHECK(Version(1, 2) == Version(1, 2, 0, 0));
        CHECK(Version(1, 2, 3) == Version(1, 2, 3, 0));
        CHECK(Version(1, 2, 3) == Version({ 1, 2, 3, 0, 0 }));
        CHECK(Version(1, 2, 3, 4) == Version({ 1, 2, 3, 4, 0 }));
        CHECK(Version(1, 2, 3, 4) == Version({1, 2, 3, 4, 0, 0}));
        CHECK(Version(1, 1, 1) == Version({1, 1, 1, 0}));
        CHECK(Version(1, 1, 1) == Version({1, 1, 1, 0, 0}));
    }

    // min, max
    /*{
        CHECK_THROWS(Version::min(-2) == Version());
        CHECK_THROWS(Version::min(-1) == Version());
        CHECK_THROWS(Version::min(0) == Version());
        CHECK(Version::min(1) == PackageVersion());
        CHECK(Version::min(2) == PackageVersion());
        CHECK(Version::min(3) == PackageVersion());
        CHECK(Version::min(4) == PackageVersion(Version(0,0,0,1)));
        CHECK(Version::min(5) == PackageVersion(Version({ 0,0,0,0,1 })));

        CHECK_THROWS(Version::max(-2) == Version(Version::maxNumber(), Version::maxNumber(), Version::maxNumber()));
        CHECK_THROWS(Version::max(-1) == Version(Version::maxNumber(), Version::maxNumber(), Version::maxNumber()));
        CHECK_THROWS(Version::max(0) == Version(Version::maxNumber(), Version::maxNumber(), Version::maxNumber()));
        CHECK(Version::max(1) == Version(Version::maxNumber(), Version::maxNumber(), Version::maxNumber()));
        CHECK(Version::max(2) == Version(Version::maxNumber(), Version::maxNumber(), Version::maxNumber()));
        CHECK(Version::max(3) == Version(Version::maxNumber(), Version::maxNumber(), Version::maxNumber()));
        CHECK(Version::max(4) == Version(Version::maxNumber(), Version::maxNumber(), Version::maxNumber(), Version::maxNumber()));
        CHECK(Version::max(5) == Version({ Version::maxNumber(), Version::maxNumber(), Version::maxNumber(), Version::maxNumber(), Version::maxNumber() }));
    }*/

    // ctors
    {
        Version v1;
        CHECK(v1 == Version(v1));
        CHECK_FALSE(v1.hasExtra());
        CHECK(v1.isRelease());
        CHECK_FALSE(v1.isPreRelease());

        Version v2(v1, "preview");
        CHECK(v2 == Version(v2));
        CHECK_FALSE(v1 == Version(v2));
        CHECK(v2.hasExtra());
        CHECK_FALSE(v2.isRelease());
        CHECK(v2.isPreRelease());

        Version v3(v1, v2.getExtra());
        CHECK(v3 == Version(v2));
        CHECK(v3 == v2);
        CHECK(v3.hasExtra());
        CHECK_FALSE(v3.isRelease());
        CHECK(v3.isPreRelease());
    }

    //
    {
        Version v1("15.9.03232.13");
        Version v2("16.0.123.13-preview");

        CHECK_FALSE(v1.hasExtra());
        CHECK(v1.isRelease());
        CHECK_FALSE(v1.isPreRelease());

        CHECK(v2.hasExtra());
        CHECK_FALSE(v2.isRelease());
        CHECK(v2.isPreRelease());

        CHECK(v1.getLevel() == 4);
        CHECK(v2.getLevel() == 4);

        /*CHECK(v1.getMatchingLevel(v1) == 4);
        CHECK(v1.getMatchingLevel(v1) == v1.getLevel());
        CHECK(v2.getMatchingLevel(v2) == 4);
        CHECK(v2.getMatchingLevel(v2) == v2.getLevel());

        CHECK(v1.getMatchingLevel(v2) == 0);
        CHECK(v2.getMatchingLevel(v1) == 0);

        CHECK(v1.getMatchingLevel(Version({15,9,3232,13,5,6 })) == 4);
        CHECK(v1.getMatchingLevel(Version({15,9,3232,13,5 })) == 4);
        CHECK(v1.getMatchingLevel(Version({15,9,3232,13 })) == 4);
        CHECK(v1.getMatchingLevel(Version({15,9,3232 })) == 3);
        CHECK(v1.getMatchingLevel(Version({15,9 })) == 2);
        CHECK(v1.getMatchingLevel(Version({15 })) == 1);
        CHECK(v1.getMatchingLevel(Version({0 })) == 0);*/
    }
}

TEST_CASE("Checking version range pairs", "[range_pair]")
{
    using namespace primitives::version;

    // unions are handled in terms of merging two intervals
    // if union contains both pairs, we result empty optional

    // [1,2] & [3,4] = {}
    // [1,2] | [3,4] = {}
    {
        detail::RangePair rp1{1,false,2,false};
        detail::RangePair rp2{3,false,4,false};
        CHECK_FALSE(rp1 & rp2);
        CHECK_FALSE(rp2 & rp1);
        CHECK_FALSE(rp1 | rp2);
        CHECK_FALSE(rp2 | rp1);
    }

    // [1,2] & [2,3] = [2,2]
    // [1,2] | [2,3] = [1,3]
    {
        detail::RangePair rp1{1,false,2,false};
        detail::RangePair rp2{2,false,3,false};
        {
            auto rp3 = rp1 & rp2;
            REQUIRE(rp3);
            CHECK(rp3->getFirst() == 2);
            CHECK(rp3->getSecond() == 2);
        }
        {
            auto rp3 = rp2 & rp1;
            REQUIRE(rp3);
            CHECK(rp3->getFirst() == 2);
            CHECK(rp3->getSecond() == 2);
        }
        {
            auto rp3 = rp1 | rp2;
            REQUIRE(rp3);
            CHECK(rp3->getFirst() == 1);
            CHECK(rp3->getSecond() == 3);
        }
        {
            auto rp3 = rp2 | rp1;
            REQUIRE(rp3);
            CHECK(rp3->getFirst() == 1);
            CHECK(rp3->getSecond() == 3);
        }
    }

    // [1,2) & [2,3] = {}
    // [1,2) | [2,3] = [1,3]
    {
        detail::RangePair rp1{1,false,2,true};
        detail::RangePair rp2{2,false,3,false};
        CHECK_FALSE(rp1 & rp2);
        CHECK_FALSE(rp2 & rp1);
        {
            auto rp3 = rp1 | rp2;
            REQUIRE(rp3);
            CHECK(rp3->getFirst() == 1);
            CHECK(rp3->getSecond() == 3);
        }
        {
            auto rp3 = rp2 | rp1;
            REQUIRE(rp3);
            CHECK(rp3->getFirst() == 1);
            CHECK(rp3->getSecond() == 3);
        }
    }

    // [1,2] & (2,3] = {}
    // [1,2] | (2,3] = {}
    {
        detail::RangePair rp1{1,false,2,false};
        detail::RangePair rp2{2,true,3,false};
        CHECK_FALSE(rp1 & rp2);
        CHECK_FALSE(rp2 & rp1);
        {
            auto rp3 = rp1 | rp2;
            REQUIRE(rp3);
            CHECK(rp3->getFirst() == 1);
            CHECK(rp3->getSecond() == 3);
        }
        {
            auto rp3 = rp2 | rp1;
            REQUIRE(rp3);
            CHECK(rp3->getFirst() == 1);
            CHECK(rp3->getSecond() == 3);
        }
    }

    // [1,2) & (2,3] = {}
    // [1,2) | (2,3] = {}
    {
        detail::RangePair rp1{1,false,2,true};
        detail::RangePair rp2{2,true,3,false};
        CHECK_FALSE(rp1 & rp2);
        CHECK_FALSE(rp2 & rp1);
        CHECK_FALSE(rp1 | rp2);
        CHECK_FALSE(rp2 | rp1);
    }
}

TEST_CASE("Checking version ranges", "[range]")
{
    using namespace primitives::version;

    // new tests
    {
        CHECK_THROWS(VersionRange("-1"));

        CHECK(VersionRange("<2").contains(Version("1")));
        CHECK(VersionRange("<2").contains(Version("0.1")));
        CHECK(VersionRange("<2").contains(Version("0.0.1")));
        CHECK(VersionRange("<2").contains(Version("0.0.0.1")));
        CHECK(VersionRange("<2").contains(Version("0.0.0.0.1")));
        CHECK_FALSE(VersionRange("<2").contains(Version("2")));

        CHECK_FALSE(VersionRange(">1").contains(Version("0")));
        CHECK_FALSE(VersionRange(">1").contains(Version("0.0")));
        CHECK_FALSE(VersionRange(">1").contains(Version("0.0.0")));
        CHECK_FALSE(VersionRange(">1").contains(Version("0.0.0.0")));
        CHECK_FALSE(VersionRange(">1").contains(Version("0.0.0.0.1")));
        CHECK_FALSE(VersionRange(">1").contains(Version("1")));
        CHECK(VersionRange(">=1").contains(Version("1")));
        CHECK(VersionRange(">1").contains(Version("1.0.0.0.0.0.1")));
        CHECK(VersionRange(">1").contains(Version("1.0.0.0.0.0.1")));
        CHECK(VersionRange(">1").contains(Version("999999999")));
        CHECK(VersionRange(">1").contains(Version("999999999.999999999.999999999")));
        CHECK(VersionRange(">1").contains(Version("999999999.999999999.999999999.999999999")));
        CHECK(VersionRange(">1").contains(Version("999999999.999999999.999999999.999999999.999999999")));
        CHECK_FALSE(VersionRange(">1").contains(Version("1000000000")));

        CHECK(VersionRange(">1 <3||>5 <7").toString() == ">1.0.0 <3.0.0 || >5.0.0 <7.0.0");
        CHECK(VersionRange(">=1 <3||>5 <=7").toString() == ">=1.0.0 <3.0.0 || >5.0.0 <=7.0.0");
        CHECK(VersionRange(">1 <3||>5 <7").toString(VersionRangePairStringRepresentationType::SameDefaultLevel) == ">1.0.0 <3.0.0 || >5.0.0 <7.0.0");
        CHECK(VersionRange(">1.1 <3||>5 <7").toString(VersionRangePairStringRepresentationType::SameDefaultLevel) == ">1.1.0 <3.0.0 || >5.0.0 <7.0.0");
        CHECK(VersionRange(">1 <3||>5 <7").toString() == VersionRange(">1 <3||>5 <7").toString(VersionRangePairStringRepresentationType::SameDefaultLevel));
        CHECK(VersionRange(">1 <3||>5 <7").toString(VersionRangePairStringRepresentationType::SameRealLevel) == ">1 <3 || >5 <7");
        CHECK(VersionRange(">1 <3||>5 <7").toString(VersionRangePairStringRepresentationType::IndividualRealLevel) == ">1 <3 || >5 <7");
        CHECK(VersionRange(">1.1 <3||>5 <7").toString(VersionRangePairStringRepresentationType::IndividualRealLevel) == ">1.1 <3 || >5 <7");
        CHECK(VersionRange(">1.1 <3.3.3.3||>5 <7").toString(VersionRangePairStringRepresentationType::IndividualRealLevel) == ">1.1 <3.3.3.3 || >5 <7");
        CHECK(VersionRange(">1.1.0 <3.3.3.3.0.0||>5.0.0.0 <7.0").toString(VersionRangePairStringRepresentationType::IndividualRealLevel) == ">1.1 <3.3.3.3 || >5 <7");
        CHECK(VersionRange(">1.1.0 <3.3.3.3.0.0||>5.0.0.0 <7.0").toString() == ">1.1.0 <3.3.3.3 || >5.0.0 <7.0.0");
    }

    // assignment
    {
        VersionRange vr1;
        VersionRange vr2;
        vr2 = " > 1 < 3 || >5 <7 ";
        vr1 = vr2;
        CHECK(vr1.toString() == ">1.0.0 <3.0.0 || >5.0.0 <7.0.0");
        CHECK(vr1.toString(VersionRangePairStringRepresentationType::SameRealLevel) == ">1 <3 || >5 <7");
        CHECK(vr1.toString(VersionRangePairStringRepresentationType::IndividualRealLevel) == ">1 <3 || >5 <7");
        vr1 |= ">2 <4||>4 <6";
        CHECK(vr1.toString() == ">1.0.0 <4.0.0 || >4.0.0 <7.0.0");
        CHECK(vr1.toString(VersionRangePairStringRepresentationType::SameRealLevel) == ">1 <4 || >4 <7");
        CHECK(vr1.toString(VersionRangePairStringRepresentationType::IndividualRealLevel) == ">1 <4 || >4 <7");
    }

    // deny version - range pairs
    {
        CHECK_THROWS(VersionRange(">1 <master"));
        CHECK_THROWS(VersionRange(">1 <master-x"));
        CHECK_THROWS(VersionRange(">master <1"));
        CHECK_THROWS(VersionRange("1 - master"));
        CHECK_THROWS(VersionRange("master - 1"));
        CHECK_THROWS(VersionRange("(master,1)"));
        CHECK_THROWS(VersionRange("[master,1)"));
        CHECK_THROWS(VersionRange("(master,1]"));
        CHECK_THROWS(VersionRange("[master,1]"));
        CHECK_THROWS(VersionRange("[master , 1)"));
        CHECK_THROWS(VersionRange("(master , 1)"));
        CHECK_THROWS(VersionRange("[master-a ,master-b]"));
        // also deny branch-branch pairs
        CHECK_THROWS(VersionRange("master - master"));
        CHECK_THROWS(VersionRange("[master ,master]"));
    }

    // check that left <= right
    {
        CHECK_THROWS(VersionRange("5 - 4"));
        CHECK_NOTHROW(VersionRange("5 - 5"));
        CHECK_NOTHROW(VersionRange("[5,5]"));
        CHECK_THROWS(VersionRange("(5,5]"));
        CHECK_THROWS(VersionRange("[5,5)"));
        CHECK_THROWS(VersionRange("(5,5)"));
        CHECK_THROWS(VersionRange("[5,4]"));
        CHECK_THROWS(VersionRange("[5,4)"));
        CHECK_THROWS(VersionRange("(5,4]"));
        CHECK_THROWS(VersionRange("(5,4)"));
        CHECK_NOTHROW(VersionRange(">2 <2"));
    }

    // VersionRange ops
    {
        VersionRange vr1(" > 1 < 3 || >5 <7 ");
        VersionRange vr2(">2 <4||>4 <6");
        vr1 |= vr2;
        CHECK(vr1.toString() == ">1.0.0 <4.0.0 || >4.0.0 <7.0.0");
    }

    {
        VersionRange vr1("<2");
        VersionRange vr2(">3");
        vr1 |= vr2;
        CHECK(vr1.toString() == "<2.0.0 || >3.0.0");
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
        CHECK(vr1.toString() == ">3.0.0 <4.0.0");
    }

    // RANGE
    {
        VersionRange vr("*");
        auto s = vr.toString();
        CHECK(s == "*");
        //CHECK(VersionRange("x").toString() == "*");
        //CHECK(VersionRange("X").toString() == "*");
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
        CHECK(s == ">=1.2.3 <1.2.4");
    }

    {
        VersionRange vr("=1.2.3");
        CHECK(vr.contains(Version("1.2.3")));
        CHECK(vr.contains(Version("1.2.3.0")));
        CHECK(vr.contains(Version("1.2.3.0.0")));
        CHECK(vr.contains(Version("1.2.3.0.0.0")));
        CHECK(vr.contains(Version("1.2.3.0.000.000")));
        CHECK_FALSE(vr.contains(Version("1.2.3.0.0.0.1")));
        CHECK_FALSE(vr.contains(Version("1.2.3.0.0.0.01")));
        CHECK_FALSE(vr.contains(Version("1.2.3.0.0.00.01")));
        CHECK(vr.toString() == "=1.2.3");
    }

    {
        VersionRange vr("!=1.2.3");
        CHECK_FALSE(vr.contains(Version("1.2.3")));
        CHECK_FALSE(vr.contains(Version("1.2.3.0")));
        CHECK_FALSE(vr.contains(Version("1.2.3.0.0")));
        CHECK_FALSE(vr.contains(Version("1.2.3.0.0.0")));
        CHECK_FALSE(vr.contains(Version("1.2.3.0.000.000")));
        CHECK(vr.contains(Version("1.2.3.0.0.0.1")));
        CHECK(vr.contains(Version("1.2.3.0.0.0.01")));
        CHECK(vr.contains(Version("1.2.3.0.0.00.01")));
        CHECK(vr.contains(Version("1.2.2.0.0.00.01")));
        CHECK(vr.toString() == "<1.2.3 || >1.2.3");
    }

    {
        VersionRange vr("1.2.3.4");
        auto s = vr.toString();
        CHECK(s == ">=1.2.3.4 <1.2.3.5");
    }

    {
        VersionRange vr("=1.2.3.4");
        auto s = vr.toString();
        CHECK(s == "=1.2.3.4");
    }

    {
        VersionRange vr("1.0.1");
        auto s = vr.toString();
        CHECK(s == ">=1.0.1 <1.0.2");
    }

    {
        VersionRange vr("1.0.0.2");
        auto s = vr.toString();
        CHECK(s == ">=1.0.0.2 <1.0.0.3");
    }

    // CMP
    {
        VersionRange vr("> 1");
        auto s = vr.toString();
        CHECK(s == ">1.0.0");
    }

    {
        VersionRange vr(">1.2");
        auto s = vr.toString();
        CHECK(s == ">1.2.0");
    }

    {
        VersionRange vr(">1.2.3");
        auto s = vr.toString();
        CHECK(s == ">1.2.3");
    }

    {
        VersionRange vr(">1.2.3.4");
        auto s = vr.toString();
        CHECK(s == ">1.2.3.4");
    }

    {
        VersionRange vr("> 1.0.0");
        auto s = vr.toString();
        CHECK(s == ">1.0.0");
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
        CHECK(s == "<1.2.3");
    }

    {
        VersionRange vr("<1.2.3.4");
        auto s = vr.toString();
        CHECK(s == "<1.2.3.4");
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
        CHECK(vr.toString() == "=1.0.0");
        CHECK(VersionRange("= 1").toString() == "=1.0.0");
    }

    {
        VersionRange vr("!= 1");
        CHECK(vr.toString() == "<1.0.0 || >1.0.0");
    }

    // TILDE
    {
        CHECK(VersionRange("~ 1").toString() == ">=1.0.0 <2.0.0");
        CHECK(VersionRange("~1").toString() == ">=1.0.0 <2.0.0");
        CHECK(VersionRange("~1.2").toString() == ">=1.2.0 <1.3.0");
        CHECK(VersionRange("~1.2.3").toString() == ">=1.2.3 <1.3.0");
        CHECK(VersionRange("~1.2.3.4").toString() == ">=1.2.3.4 <1.3.0");
    }

    // CARET
    {
        CHECK(VersionRange("^ 1").toString() == ">=1.0.0 <2.0.0");
        CHECK(VersionRange("^1").toString() == ">=1.0.0 <2.0.0");
        CHECK(VersionRange("^1.*").toString() == ">=1.0.0 <2.0.0");
        CHECK(VersionRange("^1.X").toString() == ">=1.0.0 <2.0.0");
        CHECK(VersionRange("^1.2").toString() == ">=1.2.0 <2.0.0");
        CHECK(VersionRange("^1.2.3").toString() == ">=1.2.3 <2.0.0");
        CHECK(VersionRange("^1.2.3.4").toString() == ">=1.2.3.4 <2.0.0");
        CHECK(VersionRange("^3.2.3.4").toString() == ">=3.2.3.4 <4.0.0");

        CHECK(VersionRange("^ 0").toString() == "<1.0.0");
        CHECK(VersionRange("^0").toString() == "<1.0.0");
        CHECK(VersionRange("^ 0.0").toString() == "<0.1.0");
        CHECK(VersionRange("^ 0.2").toString() == ">=0.2.0 <0.3.0");
        CHECK(VersionRange("^ 0.2.3").toString() == ">=0.2.3 <0.3.0");
        CHECK(VersionRange("^ 0.0.3").toString() == ">=0.0.3 <0.0.4");
        CHECK(VersionRange("^0.0.3.0").toString() == ">=0.0.3 <0.0.4");
        CHECK(VersionRange("^0.0.0.4").toString() == ">=0.0.0.4 <0.0.0.5");
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
        CHECK(vr.toString() == ">=1.2.3 <2.3.5");
    }

    {
        VersionRange vr("1.2.3.4 - 2");
        CHECK(vr.toString() == ">=1.2.3.4 <3.0.0");
        CHECK(vr.toString(VersionRangePairStringRepresentationType::IndividualRealLevel) == ">=1.2.3.4 <3");
    }

    {
        VersionRange vr("1.2.3.4 - 2.3.4.5");
        CHECK(vr.toString() == ">=1.2.3.4 <2.3.4.6");
    }

    // NORMALIZE
    {
        VersionRange vr(">7 >8 >9");
        CHECK(vr.toString() == ">9.0.0");
    }

    {
        VersionRange vr(">=7 >8 >9");
        CHECK(vr.toString() == ">9.0.0");
    }

    {
        VersionRange vr(">=7 >8 >=9");
        CHECK(vr.toString() == ">=9.0.0");
        CHECK(vr.toString(VersionRangePairStringRepresentationType::IndividualRealLevel) == ">=9");
    }

    {
        VersionRange vr(">7 >8 <10");
        CHECK(vr.toString() == ">8.0.0 <10.0.0");
    }

    {
        VersionRange vr(">7 ||>8 || >9");
        CHECK(vr.toString() == ">7.0.0");
    }

    {
        VersionRange vr(">=7 || >8 || >9");
        CHECK(vr.toString() == ">=7.0.0");
    }

    {
        VersionRange vr(">7||>8||<10");
        CHECK(vr.toString() == "*");
    }

    {
        Version v1("1.1");
        Version v2("1.2");
        //VersionRange vr1(v1, v2);
        //CHECK(vr1.size() == 1);

        //CHECK_THROWS(VersionRange(v2, v1));
    }

    // from simple to complex
    CHECK(VersionRange("1").toString() == ">=1.0.0 <2.0.0");
    CHECK(VersionRange("1.0.0").toString() == ">=1.0.0 <1.0.1");
    CHECK(VersionRange("1.0.0").toString(VersionRangePairStringRepresentationType::IndividualRealLevel) == ">=1 <1.0.1");
    CHECK(VersionRange("1.0.0.01").toString() == ">=1.0.0.1 <1.0.0.2");
    CHECK(VersionRange("1 - 2").toString() == ">=1.0.0 <3.0.0");
    CHECK(VersionRange("1 - 2").toString(VersionRangePairStringRepresentationType::SameRealLevel) == ">=1 <3");
    CHECK(VersionRange("1 - 2.2").toString() == ">=1.0.0 <2.3.0");
    CHECK(VersionRange("1.2 - 2").toString() == ">=1.2.0 <3.0.0");
    CHECK(VersionRange("1.2 - 2").toString(VersionRangePairStringRepresentationType::SameRealLevel) == ">=1.2 <3");
    CHECK(VersionRange("1.2 - 2").toString(VersionRangePairStringRepresentationType::IndividualRealLevel) == ">=1.2 <3");
    CHECK(VersionRange("1.2.3 - 2").toString() == ">=1.2.3 <3.0.0");
    CHECK(VersionRange("1.2.3.4 - 2").toString() == ">=1.2.3.4 <3.0.0");

    CHECK(VersionRange("").toString() == "*");

    // level ctor, do we really need it?
    /*
    CHECK_NOTHROW(VersionRange(-2));
    CHECK_NOTHROW(VersionRange(-1));
    CHECK_THROWS(VersionRange(0));
    CHECK(VersionRange(1).toString() == "*");
    CHECK(VersionRange(2).toString() == "*");
    CHECK(VersionRange(3).toString() == "*");
    //CHECK(VersionRange(4).toString() == "*.*.*.*"); // changed
    //CHECK(VersionRange(5).toString() == "*.*.*.*.*"); // changed
    CHECK(VersionRange(4).toString() == "*");
    CHECK(VersionRange(5).toString() == "*");
    CHECK(VersionRange(3).contains(Version(0,0,1)));
    CHECK_FALSE(VersionRange(3).contains(Version(0,0,0,1)));
    CHECK(VersionRange(3).contains(Version(0,0,2)));
    CHECK(VersionRange(4).contains(Version(0, 0, 1)));
    CHECK(VersionRange(4).contains(Version(0, 0, 0, 1)));
    CHECK_FALSE(VersionRange(4).contains(Version({ 0, 0, 0, 0, 1 })));
    CHECK(VersionRange(Version::getDefaultLevel()).toString() == "*");*/

    // from websites
    // https://docs.npmjs.com/misc/semver
    CHECK(VersionRange("^4.8.0 || ^5.7.0 || ^6.2.2 || >=8.0.0").toString() ==
        ">=4.8.0 <5.0.0 || >=5.7.0 <6.0.0 || >=6.2.2 <7.0.0 || >=8.0.0");
    CHECK(VersionRange("^4.8.0 || ^5.7.0 || ^6.2.2 || >8.0.0").toString() ==
        ">=4.8.0 <5.0.0 || >=5.7.0 <6.0.0 || >=6.2.2 <7.0.0 || >8.0.0");
    CHECK(VersionRange("^4.8.0 || ^5.7.0.0 || ^6.2.2 || >8.0.0.0").toString() ==
        ">=4.8.0 <5.0.0 || >=5.7.0 <6.0.0 || >=6.2.2 <7.0.0 || >8.0.0");
    CHECK(VersionRange("1.x || >=2.5.0 || 5.0.0 - 7.2.3").toString() ==
        ">=1.0.0 <2.0.0 || >=2.5.0");
    CHECK(VersionRange("1.x.x.X || >=2.5.0 || 5.0.0 - 7.2.3").toString() ==
        ">=1.0.0 <2.0.0 || >=2.5.0");
    CHECK(VersionRange("1.x.x.* || >=2.5.0 || 5.0.0 - 7.2.3").toString() ==
        ">=1.0.0 <2.0.0 || >=2.5.0");
    CHECK(VersionRange("42.6.7-alpha").toString() == ">=42.6.7-alpha <42.6.8");
    CHECK(VersionRange("42.6.7.9-alpha").toString() == ">=42.6.7.9-alpha <42.6.7.10");
    CHECK(VersionRange(">1.2.3-alpha.3").toString() == ">1.2.3-alpha.3");
    CHECK(VersionRange("1.2.3 - 2.3.4").toString() == ">=1.2.3 <2.3.5");
    CHECK(VersionRange("1.2 - 2.3.4").toString() == ">=1.2.0 <2.3.5");
    CHECK(VersionRange("1.2.x - 2.3.4").toString() == ">=1.2.0 <2.3.5");
    CHECK(VersionRange("1.2.* - 2.3.4").toString() == ">=1.2.0 <2.3.5");
    CHECK(VersionRange("1.2.3.* - 2.3.4").toString() == ">=1.2.3 <2.3.5");
    CHECK(VersionRange("   1.2.3 - 2.3    ").toString() == ">=1.2.3 <2.4.0");
    CHECK(VersionRange("1.2.3 - 2").toString() == ">=1.2.3 <3.0.0");
    CHECK(VersionRange("*").toString() == "*");
    CHECK(VersionRange("*").toString() == "*");
    CHECK(VersionRange("*.x.X.*").toString() == "*");
    CHECK(VersionRange("*.x.X.x.*").toString() == "*");
    CHECK(VersionRange("1.x").toString() == ">=1.0.0 <2.0.0");
    CHECK(VersionRange("1").toString() == ">=1.0.0 <2.0.0");
    CHECK(VersionRange("1.*").toString() == ">=1.0.0 <2.0.0");
    CHECK(VersionRange("1.*.*.*").toString() == ">=1.0.0 <2.0.0");
    CHECK(VersionRange("1.x.x.x").toString() == ">=1.0.0 <2.0.0");
    CHECK(VersionRange("1.2.x").toString() == ">=1.2.0 <1.3.0");
    CHECK(VersionRange("").toString() == "*");
    CHECK(VersionRange("1").toString() == ">=1.0.0 <2.0.0");
    CHECK(VersionRange("1.2").toString() == ">=1.2.0 <1.3.0");
    CHECK(VersionRange("~1").toString() == ">=1.0.0 <2.0.0");
    CHECK(VersionRange("~1.2").toString() == ">=1.2.0 <1.3.0");
    CHECK(VersionRange("~1.2.3").toString() == ">=1.2.3 <1.3.0");
    CHECK(VersionRange("~1.2.3.4").toString() == ">=1.2.3.4 <1.3.0");
    CHECK(VersionRange("~0").toString() == "<1.0.0");
    CHECK(VersionRange("~0.2").toString() == ">=0.2.0 <0.3.0");
    CHECK(VersionRange("~0.2.3").toString() == ">=0.2.3 <0.3.0");
    CHECK(VersionRange("~0.2.3.4").toString() == ">=0.2.3.4 <0.3.0");
    CHECK(VersionRange("^1").toString() == ">=1.0.0 <2.0.0");
    CHECK(VersionRange("^1.2").toString() == ">=1.2.0 <2.0.0");
    CHECK(VersionRange("^1.2.3").toString() == ">=1.2.3 <2.0.0");
    CHECK(VersionRange("^1.2.3.4").toString() == ">=1.2.3.4 <2.0.0");
    CHECK(VersionRange("^0").toString() == "<1.0.0");
    CHECK(VersionRange("^0.2").toString() == ">=0.2.0 <0.3.0");
    CHECK(VersionRange("^0.2.3").toString() == ">=0.2.3 <0.3.0");
    CHECK(VersionRange("^0.2.3.4").toString() == ">=0.2.3.4 <0.3.0");
    CHECK(VersionRange("^0.0.3").toString() == ">=0.0.3 <0.0.4");
    CHECK(VersionRange("^0.0.3.4").toString() == ">=0.0.3.4 <0.0.4");
    CHECK(VersionRange("^0.0.0.4").toString() == ">=0.0.0.4 <0.0.0.5");
    CHECK(VersionRange("^1.2.3-beta.2").toString() == ">=1.2.3-beta.2 <2.0.0");
    CHECK(VersionRange("^0.0.3-beta").toString() == ">=0.0.3-beta <0.0.4");
    CHECK(VersionRange("^0.0.3.4-beta").toString() == ">=0.0.3.4-beta <0.0.4");
    CHECK(VersionRange("^1.2.x").toString() == ">=1.2.0 <2.0.0");
    CHECK(VersionRange("^1.2.x.*").toString() == ">=1.2.0 <2.0.0");
    CHECK(VersionRange("^0.0.x").toString() == "<0.1.0");
    CHECK(VersionRange("^0.0").toString() == "<0.1.0");
    CHECK(VersionRange("^1.x").toString() == ">=1.0.0 <2.0.0");
    CHECK(VersionRange("^0.x").toString() == "<1.0.0");
    CHECK(VersionRange("~1.2.x").toString() == ">=1.2.0 <1.3.0");
    CHECK(VersionRange("~1.2.x.*").toString() == ">=1.2.0 <1.3.0");
    CHECK(VersionRange("~0.0.x").toString() == "<0.1.0");
    CHECK(VersionRange("~0.0").toString() == "<0.1.0");
    CHECK(VersionRange("~1.x").toString() == ">=1.0.0 <2.0.0");
    CHECK(VersionRange("~0.x").toString() == "<1.0.0");

    CHECK(VersionRange("~1.x-x").toString() == ">=1.0.0-x <2.0.0");
    CHECK(VersionRange("~1.x-X").toString() == ">=1.0.0-X <2.0.0");
    CHECK(VersionRange("~1.x-xx").toString() == ">=1.0.0-xx <2.0.0");
    CHECK(VersionRange("~1.x-XX").toString() == ">=1.0.0-XX <2.0.0");
    CHECK(VersionRange("~1.x-xxx").toString() == ">=1.0.0-xxx <2.0.0");
    CHECK(VersionRange("~1.x-XXX").toString() == ">=1.0.0-XXX <2.0.0");
    CHECK(VersionRange("~1.x-xXx").toString() == ">=1.0.0-xXx <2.0.0");
    CHECK(VersionRange("~1.x-XxX").toString() == ">=1.0.0-XxX <2.0.0");
    CHECK(VersionRange("~1.x-xax").toString() == ">=1.0.0-xax <2.0.0");
    CHECK(VersionRange("~1.x-XaX").toString() == ">=1.0.0-XaX <2.0.0");
    CHECK(VersionRange("~1.x-xxxax").toString() == ">=1.0.0-xxxax <2.0.0");
    CHECK(VersionRange("~1.x-XXXaX").toString() == ">=1.0.0-XXXaX <2.0.0");
    CHECK(VersionRange("~1.x-xXxAx").toString() == ">=1.0.0-xXxAx <2.0.0");
    CHECK(VersionRange("~1.x-XxXaX").toString() == ">=1.0.0-XxXaX <2.0.0");
    CHECK(VersionRange("~1.x-ax").toString() == ">=1.0.0-ax <2.0.0");
    CHECK(VersionRange("~1.x-aX").toString() == ">=1.0.0-aX <2.0.0");
    CHECK(VersionRange("~1.x-axa").toString() == ">=1.0.0-axa <2.0.0");
    CHECK(VersionRange("~1.x-aXa").toString() == ">=1.0.0-aXa <2.0.0");
    CHECK(VersionRange("~1.x.x.x-axa").toString() == ">=1.0.0-axa <2.0.0");
    CHECK(VersionRange("~1.x.x.x-aXa").toString() == ">=1.0.0-aXa <2.0.0");

    CHECK_THROWS(VersionRange("~0.x-x").toString() == ">=0.0.1-x <1.0.0");
    CHECK_THROWS(VersionRange("~0.x-X").toString() == ">=0.0.1-X <1.0.0");
    CHECK_THROWS(VersionRange("~0.x-xx").toString() == ">=0.0.1-xx <1.0.0");
    CHECK_THROWS(VersionRange("~0.x-XX").toString() == ">=0.0.1-XX <1.0.0");
    CHECK_THROWS(VersionRange("~0.x-xxx").toString() == ">=0.0.1-xxx <1.0.0");
    CHECK_THROWS(VersionRange("~0.x-XXX").toString() == ">=0.0.1-XXX <1.0.0");
    CHECK_THROWS(VersionRange("~0.x-xXx").toString() == ">=0.0.1-xXx <1.0.0");
    CHECK_THROWS(VersionRange("~0.x-XxX").toString() == ">=0.0.1-XxX <1.0.0");
    CHECK_THROWS(VersionRange("~0.x-xax").toString() == ">=0.0.1-xax <1.0.0");
    CHECK_THROWS(VersionRange("~0.x-XaX").toString() == ">=0.0.1-XaX <1.0.0");
    CHECK_THROWS(VersionRange("~0.x-xxxax").toString() == ">=0.0.1-xxxax <1.0.0");
    CHECK_THROWS(VersionRange("~0.x-XXXaX").toString() == ">=0.0.1-XXXaX <1.0.0");
    CHECK_THROWS(VersionRange("~0.x-xXxAx").toString() == ">=0.0.1-xXxAx <1.0.0");
    CHECK_THROWS(VersionRange("~0.x-XxXaX").toString() == ">=0.0.1-XxXaX <1.0.0");
    CHECK_THROWS(VersionRange("~0.x-ax").toString() == ">=0.0.1-ax <1.0.0");
    CHECK_THROWS(VersionRange("~0.x-aX").toString() == ">=0.0.1-aX <1.0.0");
    CHECK_THROWS(VersionRange("~0.x-axa").toString() == ">=0.0.1-axa <1.0.0");
    CHECK_THROWS(VersionRange("~0.x-aXa").toString() == ">=0.0.1-aXa <1.0.0");
    CHECK_THROWS(VersionRange("~0.x.x.x-axa").toString() == ">=0.0.0.1-axa <1.0.0.0");
    CHECK_THROWS(VersionRange("~0.x.x.x-aXa").toString() == ">=0.0.0.1-aXa <1.0.0.0");

    CHECK(VersionRange("^0.0") == VersionRange("~0.0"));

    // https://yarnpkg.com/lang/en/docs/dependency-versions/
    CHECK(VersionRange(">=2.0.0 <3.1.4").toString() == ">=2.0.0 <3.1.4");
    CHECK(VersionRange("<3.1.4 >=2.0.0 ").toString() == ">=2.0.0 <3.1.4");
    CHECK(VersionRange("<2.0.0 || >3.1.4").toString() == "<2.0.0 || >3.1.4");
    CHECK(VersionRange("^0.4.2").toString() == ">=0.4.2 <0.5.0");
    CHECK(VersionRange("~2.7.1").toString() == ">=2.7.1 <2.8.0");
    CHECK(VersionRange("<2.0.0").toString() == "<2.0.0");
    CHECK(VersionRange("<=3.1.4 ").toString() == "<=3.1.4");
    CHECK(VersionRange(">0.4.2").toString() == ">0.4.2");
    CHECK(VersionRange(">=2.7.1").toString() == ">=2.7.1");
    CHECK(VersionRange("=4.6.6").toString() == "=4.6.6");
    CHECK(VersionRange("!=4.6.6").toString() == "<4.6.6 || >4.6.6");
    CHECK(VersionRange("!=4.6.6.8").toString() == "<4.6.6.8 || >4.6.6.8");
    CHECK(VersionRange("2.0.0 - 3.1.4").toString() == ">=2.0.0 <3.1.5");
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
    CHECK(VersionRange("0.0.2").toString() == ">=0.0.2 <0.0.3");
    CHECK(VersionRange("^0.0.2").toString() == ">=0.0.2 <0.0.3");

    CHECK(VersionRange("^0.0.x").toString() == "<0.1.0");
    CHECK(VersionRange("^0.0").toString() == "<0.1.0");
    CHECK(VersionRange("^0.x").toString() == "<1.0.0");
    CHECK(VersionRange("^0").toString() == "<1.0.0");

    CHECK(VersionRange(">= 1.2, < 3.0.0 || >= 4.2.3").toString() == ">=1.2.0 <3.0.0 || >=4.2.3");
    CHECK(VersionRange(">= 1.2&&< 3.0.0 || >= 4.2.3").toString() == ">=1.2.0 <3.0.0 || >=4.2.3");
    CHECK(VersionRange("<1.1 || >= 1.2&&< 3.0.0 || >= 4.2.3").toString() == "<1.1.0 || >=1.2.0 <3.0.0 || >=4.2.3");

    // https://doc.rust-lang.org/stable/cargo/reference/specifying-dependencies.html
    CHECK(VersionRange(">= 1.2.0").toString() == ">=1.2.0");
    CHECK(VersionRange("> 1").toString() == ">1.0.0");
    CHECK(VersionRange(">= 1").toString() == ">=1.0.0");
    CHECK(VersionRange("< 2").toString() == "<2.0.0");
    CHECK(VersionRange("== 1.2.3").toString() == "=1.2.3");
    CHECK(VersionRange("= 1.2.3").toString() == "=1.2.3");
    CHECK(VersionRange("== 1.2.3.4").toString() == "=1.2.3.4");

    CHECK(VersionRange("1.2.3.4.5.6.7").toString() == ">=1.2.3.4.5.6.7 <1.2.3.4.5.6.8");
    CHECK(VersionRange("1.2.3.4.5.6.7-1").toString() == ">=1.2.3.4.5.6.7-1 <1.2.3.4.5.6.8");

    // follow
    // 'and' versions
    CHECK(VersionRange("1.0.1 1.0.2").toString() == "");
    CHECK(VersionRange("1.0.1 1.0.2 1.0.3").toString() == "");
    CHECK(VersionRange("1.0.0 1.0.1 1.0.2 1.0.3").toString() == "");
    CHECK(VersionRange("1 1.0.1 1.0.2 1.0.3").toString() == "");
    CHECK(VersionRange("1 1.0.1 1.0.2 1.0.3 - 1.1").toString() == "");

    // 'or' versions
    CHECK(VersionRange("1.0.1 || 1.0.2").toString() == ">=1.0.1 <1.0.3");
    CHECK(VersionRange("1.0.1 || 1.0.2") == VersionRange(">=1.0.1 <1.0.3"));
    CHECK(VersionRange("1.0.1 || 1.0.2 || 1.0.3").toString() == ">=1.0.1 <1.0.4");
    CHECK(VersionRange("1.0.1 || 1.0.2 || 1.0.3") == VersionRange(">=1.0.1 <1.0.4"));
    CHECK(VersionRange("1.0.0 || 1.0.1 || 1.0.2 || 1.0.3").toString() == ">=1.0.0 <1.0.4");
    CHECK(VersionRange("1.0.0 || 1.0.1 || 1.0.2 || 1.0.3") == VersionRange(">=1.0.0 <1.0.4"));
    CHECK(VersionRange("1 || 1.0.1 || 1.0.2 || 1.0.3").toString() == ">=1.0.0 <2.0.0");
    CHECK(VersionRange("1 || 1.0.1 || 1.0.2 || 1.0.3 - 2.1").toString() == ">=1.0.0 <2.2.0");

    // complex
    // at the moment whole range will have level == 4
    //CHECK(VersionRange("1.1 - 1.3 || 1.3.2.1 - 1.4").toString() == ">1.1.0 <=1.3.2 || >=1.3.2.1 <1.5.0");
    //CHECK(VersionRange("1.1 - 1.5 || 1.3.2.1 - 1.4").toString() == ">1.1.0 <=1.3.2 || >=1.3.2.1 <1.5.0");

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
    CHECK(VersionRange(" ( , 2.0.5 ) ").toString() == "<2.0.5");

    // right
    CHECK(VersionRange("[1.0,]").toString() == ">=1.0.0");
    CHECK(VersionRange(" [ 1.0 , ) ").toString() == ">=1.0.0");
    CHECK(VersionRange("(1.0,]").toString() == ">1.0.0");
    CHECK(VersionRange(" ( 1.0 , ) ").toString() == ">1.0.0");
    CHECK(VersionRange("(1,]").toString() == ">1.0.0");
    CHECK(VersionRange(" ( 1 , ) ").toString() == ">1.0.0");
    CHECK(VersionRange("(1.x,]").toString() == ">1.0.0");
    CHECK(VersionRange(" ( 1.* , ) ").toString() == ">1.0.0");
    CHECK(VersionRange("(1.5.*,]").toString() == ">1.5.0");
    CHECK(VersionRange(" ( 1.5.X , ) ").toString() == ">1.5.0");

    // both
    CHECK(VersionRange("[1,2]").toString() == ">=1.0.0 <=2.0.0");
    CHECK(VersionRange("[1,2)").toString() == ">=1.0.0 <2.0.0");
    CHECK(VersionRange("(1,2]").toString() == ">1.0.0 <=2.0.0");
    CHECK(VersionRange("(1,2)").toString() == ">1.0.0 <2.0.0");
    CHECK(VersionRange("(1.2.3,2)").toString() == ">1.2.3 <2.0.0");
    CHECK(VersionRange("(1.2.3.4,2.0.0.0)").toString() == ">1.2.3.4 <2.0.0");
    CHECK(VersionRange("(1.2.3.4,2)").toString() == ">1.2.3.4 <2.0.0");
    CHECK(VersionRange("(1,2.3.4.5)").toString() == ">1.0.0 <2.3.4.5");
    CHECK(VersionRange("(1,2.3.4.5]").toString() == ">1.0.0 <=2.3.4.5");
    CHECK(VersionRange("(1,2.3.4.5.6.7.8.9.10.11.12.13.14.15.16.17]").toString()
        == ">1.0.0 <=2.3.4.5.6.7.8.9.10.11.12.13.14.15.16.17");
    CHECK_THROWS(VersionRange("1 - 2 - 3 || 4"));

    // complex
    CHECK(VersionRange("[1,4] [2,3]").toString() == ">=2.0.0 <=3.0.0");

    // branch
    {
        CHECK_THROWS(VersionRange("master"));
        CHECK_THROWS(VersionRange("master-master"));
        CHECK_THROWS(VersionRange("master || master2"));

        CHECK_THROWS(VersionRange("a b").toString() == "a");
        CHECK_THROWS(VersionRange("a b || c").toString() == "a");
        CHECK_THROWS(VersionRange("a b - d || c").toString() == "a");
        CHECK_THROWS(VersionRange("a - b - d || c || d || e||f").toString() == "a");
    }

    {
        //VersionRange v1(Version(2, 14, 0), Version(2, 14, 0));
        //auto v = v1.getMaxSatisfyingVersion({ "2.14.0-rc16" });
        //CHECK(!v);
    }

    {
        //VersionRange v1(Version(Version(2, 14, 0), "rc16"), Version(2, 14, 0));
        //CHECK(v1.size() == 1);
        //auto v = v1.getMaxSatisfyingVersion({ "2.14.0-rc16" });
        //CHECK(v);
        //CHECK(v.value() == "2.14.0-rc16");
    }

    {
        //VersionRange v1(Version(Version(2, 14, 0), "rc16"), Version(2, 14, 0));
        //CHECK(v1.size() == 1);
        //auto v = v1.getMaxSatisfyingVersion({ "2.13.3", "2.14.0-rc16" });
        //CHECK(v);
        //CHECK(v.value() == "2.14.0-rc16");
    }

    // contains
    {
        VersionRange vr("*");
        CHECK(vr.contains(Version("1")));
        CHECK(vr.contains(Version("1.2")));
        CHECK(vr.contains(Version("1.2.3")));
        CHECK(vr.contains(Version("1.2.3.4")));
        CHECK(vr.contains(Version("1.2.3.4.5")));
        CHECK(vr.contains(Version("1.2.3.4.5-rc1"))); // change this?
        //CHECK_FALSE(vr.contains(Version("1.2.3.4.5-rc1"));
    }

    {
        VersionRange vr("*.*.*.*");
        CHECK(vr.contains(Version("1")));
        CHECK(vr.contains(Version("1.2")));
        CHECK(vr.contains(Version("1.2.3")));
        CHECK(vr.contains(Version("1.2.3.4")));
        CHECK(vr.contains(Version("1.2.3.4.5")));
    }

    {
        VersionRange vr(">=1.2.0 <=2.3.4");
        CHECK(vr.contains(Version("1.2.3")));
        CHECK(vr.contains(Version("1.2.3.4")));
        CHECK(vr.contains(Version("1.2.3.4.5")));
        CHECK(vr.contains(Version("1.2.0")));
        CHECK(vr.contains(Version("1.2.0.0")));
        CHECK(vr.contains(Version("1.2.0.0.0")));
        CHECK(vr.contains(Version("2.3.4")));
        CHECK(vr.contains(Version("2.3.4.0")));
        CHECK(vr.contains(Version("2.3.4.0.0")));
        CHECK_FALSE(vr.contains(Version("2.3.4.0.1")));
    }

    {
        VersionRange vr("1.2.0");
        CHECK(vr.contains(Version("1.2.0")));
        CHECK(vr.contains(Version("1.2.0.0")));
        CHECK_FALSE(vr.contains(Version("1.2.1")));
        CHECK(vr.contains(Version("1.2.0.1")));
        CHECK_FALSE(vr.contains(Version("1.2.3")));
        CHECK(vr.contains(Version("1.2.0.4")));
    }

    {
        VersionRange vr("1.2.0.4");
        CHECK(vr.contains(Version("1.2.0.4")));
        CHECK(vr.contains(Version("1.2.0.4.0")));
        CHECK_FALSE(vr.contains(Version("1.2.0.5")));
        CHECK(vr.contains(Version("1.2.0.4.1")));
    }

    {
        VersionRange vr("=1.2.0.4");
        CHECK(vr.contains(Version("1.2.0.4")));
        CHECK(vr.contains(Version("1.2.0.4.0")));
        CHECK_FALSE(vr.contains(Version("1.2.0.4.0.1")));
        CHECK_FALSE(vr.contains(Version("1.2.0.5")));
        CHECK_FALSE(vr.contains(Version("1.2.0.4.1")));
    }

    {
        auto check = [](const auto &vr)
        {
            CHECK(vr.contains(Version("1")));
            CHECK(vr.contains(Version("1.0")));
            CHECK(vr.contains(Version("1.0.0")));
            CHECK(vr.contains(Version("1.0.0000")));
            CHECK(vr.contains(Version("1.0.0.0")));
            CHECK(vr.contains(Version("1.0.0.0.0")));
            CHECK(vr.contains(Version("1.0.0.0.0.0")));
            CHECK_FALSE(vr.contains(Version("1.1")));
            CHECK_FALSE(vr.contains(Version("1.0.1")));
            CHECK_FALSE(vr.contains(Version("1.0.0.1")));
            CHECK_FALSE(vr.contains(Version("1.0.0.0.1")));
            CHECK_FALSE(vr.contains(Version("1.0.0.0.0.1")));
            CHECK_FALSE(vr.contains(Version("1.0.0.0.0.0.1")));
        };
        check(VersionRange("=1"));
        check(VersionRange("==1"));
        CHECK_THROWS(VersionRange("===1"));
    }

    {
        auto check = [](const auto &vr)
        {
            CHECK(vr.contains(Version("1")));
            CHECK(vr.contains(Version("1.0")));
            CHECK(vr.contains(Version("1.0.0")));
            CHECK(vr.contains(Version("1.0.0000")));
            CHECK(vr.contains(Version("1.0.0.0")));
            CHECK(vr.contains(Version("1.0.0.0.0")));
            CHECK(vr.contains(Version("1.0.0.0.0.0")));
            CHECK(vr.contains(Version("1.1")));
            CHECK(vr.contains(Version("1.0.1")));
            CHECK(vr.contains(Version("1.0.0.1")));
            CHECK(vr.contains(Version("1.0.0.0.1")));
            CHECK(vr.contains(Version("1.0.0.0.0.1")));
            CHECK(vr.contains(Version("1.0.0.0.0.0.1")));
        };
        check(VersionRange("1"));
    }

    // contains range
    {
        VersionRange vr("*");
        CHECK(vr.contains(VersionRange("1")));
        CHECK(vr.contains(VersionRange("1.2")));
        CHECK(vr.contains(VersionRange("1.2.3")));
        CHECK(vr.contains(VersionRange("1.2.3.4")));
        CHECK(vr.contains(VersionRange("1.2.3.4.5")));
        CHECK(vr.contains(VersionRange("1.2.3.4.5-rc1"))); // change this?
        //CHECK_FALSE(vr.contains("1.2.3.4.5-rc1"));
    }

    {
        VersionRange vr("1");
        CHECK(vr.contains(VersionRange("1")));
        CHECK(vr.contains(VersionRange("1.2")));
        CHECK(vr.contains(VersionRange("1.2.3")));
        CHECK(vr.contains(VersionRange("1.2.3.4")));
        CHECK(vr.contains(VersionRange("1.2.3.4.5")));
        CHECK(vr.contains(VersionRange("1.2.3.4.5-rc1"))); // change this?
    }

    {
        VersionRange vr("1");
        CHECK_FALSE(vr.contains(VersionRange("2")));
        CHECK_FALSE(vr.contains(VersionRange("2.2")));
        CHECK_FALSE(vr.contains(VersionRange("2.2.3")));
        CHECK_FALSE(vr.contains(VersionRange("2.2.3.4")));
        CHECK_FALSE(vr.contains(VersionRange("2.2.3.4.5")));
        CHECK_FALSE(vr.contains(VersionRange("2.2.3.4.5-rc1"))); // change this?
    }
}

TEST_CASE("Checking version helpers", "[helpers]")
{
    using namespace primitives::version;

    // findBestMatch
    /*auto test_findBestMatch_forward = [](auto vs, auto add, auto b, auto e)
    {
        Version v1("15.9.03232.13");
        Version v2("16.0.123.13-preview");

        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), v1) == (vs.*e)());
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), v2) == (vs.*e)());

        add(vs, v1);
        add(vs, v2);

        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), v1) == (vs.*b)());
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), "15") == (vs.*b)());
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), "15.9") == (vs.*b)());
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), "15.9.03232") == (vs.*b)());
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), "15.9.03232.13") == (vs.*b)());
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), { 15,9,3232,13,5 }) == (vs.*b)());
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), { 15,9,3232,13,5,6 }) == (vs.*b)());

        auto v2i = std::next((vs.*b)());
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), v2) == v2i);
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), "16") == (vs.*e)());
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), "16.0") == (vs.*e)());
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), "16.9") == (vs.*e)());
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), "16.0.123") == (vs.*e)());
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), "16.0.123.13") == (vs.*e)());
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), { 16,0,3232,13,5 }) == (vs.*e)());
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), { 16,9,3232,13,5,6 }) == (vs.*e)());

        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), v2) == v2i);
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), "16-pre") == v2i);
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), "16.0-x") == v2i);
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), "16.9", true) == v2i);
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), "16.0.123", true) == v2i);
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), "16.0.123.13-aaaa.1.e") == v2i);
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), { 16,0,3232,13,5 }, true) == v2i);
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), { 16,9,3232,13,5,6 }, true) == v2i);

        add(vs, { 16,9,3232,13,5,6 });

        v2i = std::next((vs.*b)());
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), v2) == v2i);
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), "16") == std::prev((vs.*e)()));
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), "16.0") == std::prev((vs.*e)()));
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), "16.0-x") == v2i);
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), "16.9") == std::prev((vs.*e)()));
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), "16.0.123") == std::prev((vs.*e)()));
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), "16.0.123.13") == std::prev((vs.*e)()));
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), { 16,0,3232,13,5 }) == std::prev((vs.*e)()));
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), { 16,9,3232,13,5,6 }) == std::prev((vs.*e)()));

        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), {}) == (vs.*e)());
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), "0") == (vs.*e)());
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), "0.9") == (vs.*e)());
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), "0.9.03232") == (vs.*e)());
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), "0.9.03232.13") == (vs.*e)());
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), { 0,9,3232,13,5 }) == (vs.*e)());
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), { 0,9,3232,13,5,6 }) == (vs.*e)());

        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), "14") == (vs.*e)());
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), "14.9") == (vs.*e)());
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), "14.9.03232") == (vs.*e)());
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), "14.9.03232.13") == (vs.*e)());
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), { 14,9,3232,13,5 }) == (vs.*e)());
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), { 14,9,3232,13,5,6 }) == (vs.*e)());

        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), "17") == (vs.*e)());
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), "17.9") == (vs.*e)());
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), "17.9.03232") == (vs.*e)());
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), "17.9.03232.13") == (vs.*e)());
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), { 17,9,3232,13,5 }) == (vs.*e)());
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), { 17,9,3232,13,5,6 }) == (vs.*e)());
    };

    auto test_findBestMatch_reverse = [](auto vs, auto add, auto b, auto e)
    {
        Version v1("15.9.03232.13");
        Version v2("16.0.123.13-preview");

        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), v1) == (vs.*e)());
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), v2) == (vs.*e)());

        add(vs, v1);
        add(vs, v2);

        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), v1) == std::next((vs.*b)()));
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), "15") == std::next((vs.*b)()));
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), "15.9") == std::next((vs.*b)()));
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), "15.9.03232") == std::next((vs.*b)()));
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), "15.9.03232.13") == std::next((vs.*b)()));
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), { 15,9,3232,13,5 }) == std::next((vs.*b)()));
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), { 15,9,3232,13,5,6 }) == std::next((vs.*b)()));

        auto v2i = (vs.*b)();
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), v2) == v2i);
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), "16") == (vs.*e)());
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), "16.0") == (vs.*e)());
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), "16.9") == (vs.*e)());
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), "16.0.123") == (vs.*e)());
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), "16.0.123.13") == (vs.*e)());
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), { 16,0,3232,13,5 }) == (vs.*e)());
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), { 16,9,3232,13,5,6 }) == (vs.*e)());

        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), v2) == v2i);
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), "16-pre") == v2i);
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), "16.0-x") == v2i);
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), "16.9", true) == v2i);
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), "16.0.123", true) == v2i);
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), "16.0.123.13-aaaa.1.e") == v2i);
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), { 16,0,3232,13,5 }, true) == v2i);
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), { 16,9,3232,13,5,6 }, true) == v2i);

        add(vs, { 16,9,3232,13,5,6 });

        v2i = std::next((vs.*b)());
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), v2) == v2i);
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), "16") == (vs.*b)());
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), "16.0") == (vs.*b)());
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), "16.0-x") == v2i);
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), "16.9") == (vs.*b)());
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), "16.0.123") == (vs.*b)());
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), "16.0.123.13") == (vs.*b)());
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), { 16,0,3232,13,5 }) == (vs.*b)());
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), { 16,9,3232,13,5,6 }) == (vs.*b)());

        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), {}) == (vs.*e)());
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), "0") == (vs.*e)());
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), "0.9") == (vs.*e)());
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), "0.9.03232") == (vs.*e)());
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), "0.9.03232.13") == (vs.*e)());
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), { 0,9,3232,13,5 }) == (vs.*e)());
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), { 0,9,3232,13,5,6 }) == (vs.*e)());

        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), "14") == (vs.*e)());
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), "14.9") == (vs.*e)());
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), "14.9.03232") == (vs.*e)());
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), "14.9.03232.13") == (vs.*e)());
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), { 14,9,3232,13,5 }) == (vs.*e)());
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), { 14,9,3232,13,5,6 }) == (vs.*e)());

        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), "17") == (vs.*e)());
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), "17.9") == (vs.*e)());
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), "17.9.03232") == (vs.*e)());
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), "17.9.03232.13") == (vs.*e)());
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), { 17,9,3232,13,5 }) == (vs.*e)());
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), { 17,9,3232,13,5,6 }) == (vs.*e)());
    };

    auto test_findBestMatch_reverse_my = [](auto vs, auto add, auto b, auto e)
    {
        Version v1("15.9.03232.13");
        Version v2("16.0.123.13-preview");

        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), v1) == (vs.*e)());
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), v2) == (vs.*e)());

        add(vs, v1);
        add(vs, v2);

        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), v1) == (vs.*b)());
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), "15") == (vs.*b)());
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), "15.9") == (vs.*b)());
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), "15.9.03232") == (vs.*b)());
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), "15.9.03232.13") == (vs.*b)());
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), { 15,9,3232,13,5 }) == (vs.*b)());
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), { 15,9,3232,13,5,6 }) == (vs.*b)());

        auto v2i = (vs.*b)();
        // begin is already after 16-preview!
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), v2) == (vs.*e)());
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), "16") == (vs.*e)());
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), "16.0") == (vs.*e)());
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), "16.9") == (vs.*e)());
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), "16.0.123") == (vs.*e)());
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), "16.0.123.13") == (vs.*e)());
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), { 16,0,3232,13,5 }) == (vs.*e)());
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), { 16,9,3232,13,5,6 }) == (vs.*e)());

        // begin is already after 16-preview!
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), v2) == (vs.*e)());
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), "16-pre") == (vs.*e)());
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), "16.0-x") == (vs.*e)());
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), "16.9", true) == (vs.*e)());
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), "16.0.123", true) == (vs.*e)());
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), "16.0.123.13-aaaa.1.e") == (vs.*e)());
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), { 16,0,3232,13,5 }, true) == (vs.*e)());
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), { 16,9,3232,13,5,6 }, true) == (vs.*e)());

        add(vs, { 16,9,3232,13,5,6 });

        v2i = std::next((vs.*b)());
        CHECK(v2i == std::prev((vs.*e)()));
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), v2) == (vs.*b)()); //
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), "16") == (vs.*b)());
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), "16.0") == (vs.*b)());
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), "16.0-x") == (vs.*b)()); //
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), "16.9") == (vs.*b)());
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), "16.0.123") == (vs.*b)());
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), "16.0.123.13") == (vs.*b)());
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), { 16,0,3232,13,5 }) == (vs.*b)());
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), { 16,9,3232,13,5,6 }) == (vs.*b)());

        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), {}) == (vs.*e)());
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), "0") == (vs.*e)());
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), "0.9") == (vs.*e)());
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), "0.9.03232") == (vs.*e)());
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), "0.9.03232.13") == (vs.*e)());
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), { 0,9,3232,13,5 }) == (vs.*e)());
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), { 0,9,3232,13,5,6 }) == (vs.*e)());

        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), "14") == (vs.*e)());
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), "14.9") == (vs.*e)());
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), "14.9.03232") == (vs.*e)());
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), "14.9.03232.13") == (vs.*e)());
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), { 14,9,3232,13,5 }) == (vs.*e)());
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), { 14,9,3232,13,5,6 }) == (vs.*e)());

        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), "17") == (vs.*e)());
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), "17.9") == (vs.*e)());
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), "17.9.03232") == (vs.*e)());
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), "17.9.03232.13") == (vs.*e)());
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), { 17,9,3232,13,5 }) == (vs.*e)());
        CHECK(findBestMatch((vs.*b)(), (vs.*e)(), { 17,9,3232,13,5,6 }) == (vs.*e)());
    };

    auto add_to_set = [](auto &a, const Version &v) { a.insert(v); };
    auto add_to_map = [](auto &a, const Version &v) { a[v]; };

    auto test_one = [&test_findBestMatch_forward, &test_findBestMatch_reverse,
                     &add_to_set, &add_to_map](auto s, auto m, auto um)
    {
        SECTION("set")
        {
            using C = decltype(s);
            C a;

            using BE = typename C::const_iterator(C::*)() const;
            using RBE = typename C::const_reverse_iterator(C::*)() const;
            BE b = &C::begin;
            BE e = &C::end;
            RBE rb = &C::rbegin;
            RBE re = &C::rend;

            test_findBestMatch_forward(a, add_to_set, b, e);
            test_findBestMatch_reverse(a, add_to_set, rb, re);
        }

        SECTION("map")
        {
            using C = decltype(m);
            C a;

            using BE = typename C::const_iterator(C::*)() const;
            using RBE = typename C::const_reverse_iterator(C::*)() const;
            BE b = &C::begin;
            BE e = &C::end;
            RBE rb = &C::rbegin;
            RBE re = &C::rend;

            test_findBestMatch_forward(a, add_to_map, b, e);
            test_findBestMatch_reverse(a, add_to_map, rb, re);
        }

        SECTION("unordered_map")
        {
            using C = decltype(um);
            C a;

            using BE = typename C::const_iterator(C::*)() const;
            BE b = &C::begin;
            BE e = &C::end;

            test_findBestMatch_forward(a, add_to_map, b, e);
            // no reverse, um does not have
        }
    };

    SECTION("standard")
    {
        test_one(std::set<Version>{}, std::map<Version, int>{}, std::unordered_map<Version, int>{});
    }

    SECTION("primitives")
    {
        test_one(VersionSet{}, VersionMap<int>{}, UnorderedVersionMap<int>{});

        SECTION("set")
        {
            using C = VersionSet;
            C a;

            using BE = C::const_iterator_releases(C::*)() const;
            using RBE = C::const_reverse_iterator_releases(C::*)() const;
            BE b = &C::begin_releases;
            BE e = &C::end_releases;
            RBE rb = &C::rbegin_releases;
            RBE re = &C::rend_releases;

            test_findBestMatch_forward(a, add_to_set, b, e); // matches common forward! :)
            test_findBestMatch_reverse_my(a, add_to_set, rb, re);
        }

        SECTION("map")
        {
            using C = VersionMap<int>;
            C a;

            using BE = C::const_iterator_releases(C::*)() const;
            using RBE = C::const_reverse_iterator_releases(C::*)() const;
            BE b = &C::begin_releases;
            BE e = &C::end_releases;
            RBE rb = &C::rbegin_releases;
            RBE re = &C::rend_releases;

            test_findBestMatch_forward(a, add_to_map, b, e);
            test_findBestMatch_reverse_my(a, add_to_map, rb, re);
        }

        SECTION("unordered_map")
        {
            using C = UnorderedVersionMap<int>;
            C a;

            using BE = C::const_iterator_releases(C::*)() const;
            BE b = &C::begin_releases;
            BE e = &C::end_releases;

            test_findBestMatch_forward(a, add_to_map, b, e);
            // no reverse, um does not have
        }
    }*/
}

TEST_CASE("Other stuff", "[stuff]")
{
    using namespace primitives::version;

    auto mn = std::to_string(Version::maxNumber());
    VersionRange vr1("1");
    VersionRange vr2("1.*");

    VersionRange vr3("1.0.*");
    CHECK(vr3.contains(Version("1.0." + mn)));
    // 1.0.* contains! 1.0.*.1
    // 1.0.* contains! 1.0.*.0.1
    CHECK(vr3.contains(Version("1.0." + mn + ".1")));
    CHECK(vr3.contains(Version("1.0." + mn + ".0.1")));

    VersionRange vr4("1.0.0");
    // 1.0.0 contains! 1.0.0.1
    // 1.0.0 contains! 1.0.0.0.1
    CHECK(vr4.contains(Version("1.0.0.1")));
    CHECK(vr4.contains(Version("1.0.0.0.1")));

    VersionRange vr5("1.0.0.*");
    // 1.0.0.* contains! 1.0.0.*.1
    // 1.0.0.* contains! 1.0.0.*.0.1
    CHECK(vr5.contains(Version("1.0.0." + mn + ".1")));
    CHECK(vr5.contains(Version("1.0.0." + mn + ".0.1")));
}

TEST_CASE("Package version ranges", "[package_version_range]")
{
    using namespace primitives::version;

    /*CHECK(PackageVersionRange().toString() == "*");

    // branches
    {
        PackageVersionRange vr("master");
        CHECK(vr.toString() == "master");
        CHECK(vr.contains(PackageVersion("master")));
        CHECK_FALSE(vr.contains(PackageVersion("master1")));
    }

    {
        //PackageVersion v("master");
        //PackageVersion v2("master2");
        //Version v3;
        //CHECK_THROWS(VersionRange(v, v2));
        //CHECK_THROWS(VersionRange(v, v3));
        //CHECK_THROWS(VersionRange(v3, v));
        //CHECK_NOTHROW(VersionRange(v, v));
        //CHECK_NOTHROW(VersionRange(v2, v2));
        //VersionRange vr(v, v);
        //CHECK(vr.contains(Version("master")));
        //CHECK_FALSE(vr.contains(Version("master2")));
    }

    // cmp
    CHECK(PackageVersionRange("a") == PackageVersionRange("a"));
    CHECK_FALSE(PackageVersionRange("a") != PackageVersionRange("a"));
    //CHECK(VersionRange() < VersionRange("a"));
    CHECK(PackageVersionRange() != PackageVersionRange("a"));
    //CHECK_THROWS(VersionRange("a") < VersionRange("b || c d"));*/
}
