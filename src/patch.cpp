// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <primitives/patch.h>
#include <primitives/sw/main.h>

#include <chrono>
#include <iostream>

#define CATCH_CONFIG_RUNNER
#include <catch2/catch.hpp>

#define REQUIRE_TIME(e, t)                                                                       \
    do                                                                                           \
    {                                                                                            \
        auto t1 = get_time([&] { REQUIRE(e); });                                                 \
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

TEST_CASE("Checking patch", "[patch]")
{
    using namespace primitives::patch;

    String in = R"(This part of the
document has stayed the
same from version to
version.  It shouldn't
be shown if it doesn't
change.  Otherwise, that
would not be helping to
compress the size of the
changes.

This paragraph contains
text that is outdated.
It will be deleted in the
near future.

It is important to spell
check this dokument. On
the other hand, a
misspelled word isn't
the end of the world.
Nothing in the rest of
this paragraph needs to
be changed. Things can
be added after it.)";

    String out = R"(This is an important
notice! It should
therefore be located at
the beginning of this
document!

This part of the
document has stayed the
same from version to
version.  It shouldn't
be shown if it doesn't
change.  Otherwise, that
would not be helping to
compress the size of the
changes.

It is important to spell
check this document. On
the other hand, a
misspelled word isn't
the end of the world.
Nothing in the rest of
this paragraph needs to
be changed. Things can
be added after it.

This paragraph contains
important new additions
to this document.)";

    const String diff = R"(--- /path/to/original	timestamp
+++ /path/to/new	timestamp
@@ -1,3 +1,9 @@
+This is an important
+notice! It should
+therefore be located at
+the beginning of this
+document!
+
 This part of the
 document has stayed the
 same from version to
@@ -8,13 +14,8 @@
 compress the size of the
 changes.

-This paragraph contains
-text that is outdated.
-It will be deleted in the
-near future.
-
 It is important to spell
-check this dokument. On
+check this document. On
 the other hand, a
 misspelled word isn't
 the end of the world.
@@ -22,3 +23,7 @@
 this paragraph needs to
 be changed. Things can
 be added after it.
+
+This paragraph contains
+important new additions
+to this document.)";

    // basic
    {
        auto r = patch(in, diff);
        REQUIRE(r.first);
        CHECK(r.second == out);
    }

    // check rest of the file saving
    {
        in += "\n";
        out += "\n";
        auto r = patch(in, diff);
        REQUIRE(r.first);
        CHECK(r.second == out);
    }
}

int main(int argc, char *argv[])
{
    auto r = Catch::Session().run(argc, argv);
    return r;
}
