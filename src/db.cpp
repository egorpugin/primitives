// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#define NOMINMAX
#include <primitives/command.h>
#include <primitives/date_time.h>
#include <primitives/db.h>
#include <primitives/schema_manager.h>
#include <primitives/db/sqlite3.h>
#include <primitives/db/postgresql.h>
#include <primitives/executor.h>
#include <primitives/filesystem.h>
#include <primitives/hash.h>
#include <primitives/sw/main.h>
#include <primitives/sw/cl.h>

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

TEST_CASE("Checking db", "[db]")
{
    using namespace primitives::db;
    using namespace primitives::db::sqlite3;
    using namespace primitives::db::postgresql;

    auto test = [](std::vector<LiteDatabase*> &sdb, const String &type)
    {
        StringDatabaseSchemaManager m;
        m.database = sdb[1];
        m.latestSchema = "CREATE TABLE a (a " + type + ")";

        // create db 1
        REQUIRE_NOTHROW(m.createOrUpdate());
        REQUIRE_NOTHROW(m.createOrUpdate());

        // create db 2
        m.database = sdb[2];
        REQUIRE_NOTHROW(m.createOrUpdate());
        REQUIRE_NOTHROW(m.createOrUpdate());
        m.database = sdb[1];

        // ...

        m.latestSchema = R"xxx(
CREATE TABLE a (a )xxx" + type + R"xxx();
CREATE TABLE b (b )xxx" + type + R"xxx();
)xxx";
        m.diffSqls = {
            "CREATE TABLE b (b " + type + ")"
        };

        // update db 1
        REQUIRE_NOTHROW(m.createOrUpdate());
        REQUIRE_NOTHROW(m.createOrUpdate());

        // create db 3
        m.database = sdb[3];
        REQUIRE_NOTHROW(m.createOrUpdate());
        REQUIRE_NOTHROW(m.createOrUpdate());
        m.database = sdb[1];

        // ...

        m.latestSchema = R"xxx(
CREATE TABLE a (a )xxx" + type + R"xxx();
CREATE TABLE b (b )xxx" + type + R"xxx();
CREATE TABLE c (c )xxx" + type + R"xxx();
)xxx";
        m.diffSqls = {
            "CREATE TABLE b (b " + type + ")",
            "CREATE TABLE c (c " + type + ")",
        };

        // update db 1
        REQUIRE_NOTHROW(m.createOrUpdate());
        REQUIRE_NOTHROW(m.createOrUpdate());

        // update db 2
        m.database = sdb[2];
        REQUIRE_NOTHROW(m.createOrUpdate());
        REQUIRE_NOTHROW(m.createOrUpdate());
        m.database = sdb[1];

        // update db 3
        m.database = sdb[3];
        REQUIRE_NOTHROW(m.createOrUpdate());
        REQUIRE_NOTHROW(m.createOrUpdate());
        m.database = sdb[1];

        // ...

        m.latestSchema = R"xxx(
CREATE TABLE a (a )xxx" + type + R"xxx();
CREATE TABLE b (b )xxx" + type + R"xxx();
CREATE TABLE c (c )xxx" + type + R"xxx();
CREATE TABLE d (d )xxx" + type + R"xxx();
)xxx";
        m.diffSqls = {
            "CREATE TABLE b (b " + type + ")",
            "CREATE TABLE c (c " + type + ")",
            "CREATE TABLE d (d " + type + ")",
        };

        // update db 1
        REQUIRE_NOTHROW(m.createOrUpdate());
        REQUIRE_NOTHROW(m.createOrUpdate());

        // update db 2
        m.database = sdb[2];
        REQUIRE_NOTHROW(m.createOrUpdate());
        REQUIRE_NOTHROW(m.createOrUpdate());
        m.database = sdb[1];

        // update db 3
        m.database = sdb[3];
        REQUIRE_NOTHROW(m.createOrUpdate());
        REQUIRE_NOTHROW(m.createOrUpdate());
        m.database = sdb[1];

        // create db 4
        m.database = sdb[4];
        REQUIRE_NOTHROW(m.createOrUpdate());
        REQUIRE_NOTHROW(m.createOrUpdate());
        m.database = sdb[1];

        REQUIRE(sdb[1]->getValue<int>("version") == 3);
        REQUIRE(sdb[2]->getValue<int>("version") == 3);
        REQUIRE(sdb[3]->getValue<int>("version") == 3);
        REQUIRE(sdb[4]->getValue<int>("version") == 3);

        sdb[1]->setValue<int>("version", 0);
        REQUIRE_THROWS(m.createOrUpdate());

        sdb[1]->setValue<int>("version", 1);
        REQUIRE_THROWS(m.createOrUpdate());

        sdb[1]->setValue<int>("version", 2);
        REQUIRE_THROWS(m.createOrUpdate());

        sdb[1]->setValue<int>("version", 3);
        REQUIRE_NOTHROW(m.createOrUpdate());

        REQUIRE_THROWS(sdb[1]->setValue<int>("-", 3));
        REQUIRE_THROWS(sdb[1]->setValue<double>("'", 3));
        REQUIRE_THROWS(sdb[1]->setValue<String>("85", ""));
        REQUIRE_THROWS(sdb[1]->setValue<String>("8a", ""));
        REQUIRE_THROWS(sdb[1]->getValue<String>("abc-"));

        REQUIRE(!sdb[1]->getValue<String>("abc_85"));
        REQUIRE_NOTHROW(sdb[1]->setValue<String>("_a8_123asd", "123"));
        REQUIRE(sdb[1]->getValue<String>("_a8_123asd") == "123");
        REQUIRE_NOTHROW(sdb[1]->setValue<double>("_a8_123asd1", 2.5));
        REQUIRE(sdb[1]->getValue<double>("_a8_123asd1") == 2.5);

        REQUIRE(!sdb[1]->getValue<int>("versionxxx"));
        REQUIRE(!sdb[1]->getValue<int>("_a8_123asd2"));

        REQUIRE_NOTHROW(sdb[1]->setValue("xxx1", 1));
        REQUIRE_NOTHROW(sdb[1]->setValue("xxx2", 11.234));
        REQUIRE_NOTHROW(sdb[1]->setValue("xxx3", "123"));
        REQUIRE(sdb[1]->getValue<String>("xxx3") == "123");
        REQUIRE_NOTHROW(sdb[1]->setValue("xxx3", "123"s));
        REQUIRE(sdb[1]->getValue<String>("xxx3") == "123"s);

        REQUIRE(sdb[1]->getValue<String>("xxx3", "x") == "123"s);
        REQUIRE(sdb[1]->getValue<String>("xxx4", "x") == "x"s);
        REQUIRE(sdb[1]->getValue("xxx4", "123"s) == "x"s);
        REQUIRE_THROWS(sdb[1]->getValue("xxx4", 0) == 0); // different types
        REQUIRE(sdb[1]->getValue("xxx5", 0) == 0);
        REQUIRE(sdb[1]->getValue("xxx6", 5.5) == 5.5);
        REQUIRE(sdb[1]->getValue<double>("xxx6", 0) == 5.5);

        REQUIRE(sdb[1]->getValue("xxx7", 0) == 0);
        REQUIRE(sdb[1]->getValue<int>("xxx7") == 0);

        REQUIRE(sdb[1]->getValue("xxx8", 10) == 10);
        REQUIRE(sdb[1]->getValue<int>("xxx8") == 10);

        auto fm = m.write("1");
        REQUIRE(fm.getDiffSqlSize() == 3);
        write_file(fm.getDiffSqlFilename(fm.getDiffSqlSize()), "CREATE TABLE e (e " + type + ")");
        REQUIRE(fm.getDiffSqlSize() == 4);
        fm.database = sdb[1];

        REQUIRE_NOTHROW(fm.createOrUpdate());
        REQUIRE_NOTHROW(fm.createOrUpdate());
        REQUIRE_THROWS(sdb[1]->execute("CREATE TABLE e (e " + type + ")"));

        m = fm.read(fm.diffSqlsDir);
        REQUIRE(m.getDiffSqlSize() == 4);

        write_file(fm.getDiffSqlFilename(fm.getDiffSqlSize()), "CREATE TABLE f (f " + type + ")");
        REQUIRE_NOTHROW(createOrUpdateSchema(*sdb[1], fm.latestSchemaFilename, path("1")));
        REQUIRE_THROWS(sdb[1]->execute("CREATE TABLE f (f " + type + ")"));

        REQUIRE_NOTHROW(fm.createOrUpdate());
        REQUIRE_NOTHROW(fm.createOrUpdate());

        REQUIRE(fm.getDiffSqlSize() == 5);
    };

    const int n = 4;

    SECTION("sqlite3")
    {
        auto get_db = [](int n)
        {
            return std::to_string(n) + ".db";
        };

        int i = 0;
        while (1)
        {
            auto db = get_db(i++);
            if (!fs::exists(db))
                break;
            fs::remove(db);
        }

        std::vector<SqliteDatabase> sdb;
        for (int i = 0; i < n + 1; i++)
            sdb.emplace_back(get_db(i));

        std::vector<LiteDatabase*> sdbp;
        for (int i = 0; i < n + 1; i++)
            sdbp.emplace_back(&sdb[i]);

        test(sdbp, "INTEGER");
    }

    SECTION("postgres")
    {
        for (int i = 0; i < n + 1; i++)
        {
            try {
                dropDb("user=postgres password=postgres", "testdb" + std::to_string(i));
            } catch (...) {}
            createDb("user=postgres password=postgres", "testdb" + std::to_string(i), "testuser");
        }

        std::vector<PostgresqlDatabase> pgdb;
        for (int i = 0; i < n + 1; i++)
            pgdb.emplace_back("dbname=testdb" + std::to_string(i) + " user=testuser password=testuser");

        std::vector<LiteDatabase*> pgdbp;
        for (int i = 0; i < n + 1; i++)
            pgdbp.emplace_back(&pgdb[i]);

        test(pgdbp, "INT");
    }
}

int main(int argc, char *argv[])
{
    auto r = Catch::Session().run(argc, argv);
    return r;
}
