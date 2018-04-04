// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <primitives/db/sqlite3.h>

#include <sqlite3.h>

namespace primitives::db::sqlite3
{

SqliteDatabase::SqliteDatabase(const path &fn)
{
    sqlite3_open(fn.string().c_str(), &db);
}

SqliteDatabase::SqliteDatabase(SqliteDatabase &&rhs)
{
    db = rhs.db;
    rhs.db = nullptr;
}

SqliteDatabase::~SqliteDatabase()
{
    sqlite3_close(db);
}

void SqliteDatabase::execute(const String &q)
{
    auto r = sqlite3_exec(db, q.c_str(), 0, 0, 0);
    check_error(r, SQLITE_OK);
}

void SqliteDatabase::check_error(int r, int ec)
{
    if (r == ec)
        return;
    auto err = sqlite3_errmsg(db);
    throw std::runtime_error("db error: "s + err);
}

optional<String> SqliteDatabase::getValueByKey(const String &key)
{
    createServiceTable();
    if (createServiceTableColumn(key) == SQLITE_OK)
        return {};

#define CHECK_RC(rc, ec) if (r != ec) return {}

    int r = 0;
    auto q = "SELECT " + key + " FROM " + service_table;
    sqlite3_stmt *s;
    r = sqlite3_prepare_v2(db, q.c_str(), q.size(), &s, 0);
    CHECK_RC(r, SQLITE_OK);
    r = sqlite3_step(s);
    CHECK_RC(r, SQLITE_ROW);
    auto v = sqlite3_column_text(s, 0);
    String val = v ? (char*)v : "";
    r = sqlite3_step(s);
    CHECK_RC(r, SQLITE_DONE);
    r = sqlite3_finalize(s);
    CHECK_RC(r, SQLITE_OK);
    return val;

#undef CHECK_RC
}

void SqliteDatabase::setValueByKey(const String &key, const String &value)
{
    createServiceTable();
    createServiceTableColumn(key);

    int r = 0;
    auto q = "UPDATE " + service_table + " SET " + key + " = ?";
    sqlite3_stmt *s;
    r = sqlite3_prepare_v2(db, q.c_str(), q.size(), &s, 0);
    check_error(r, SQLITE_OK);
    r = sqlite3_bind_text(s, 1, value.c_str(), -1, 0);
    check_error(r, SQLITE_OK);
    r = sqlite3_step(s);
    check_error(r, SQLITE_DONE);
    r = sqlite3_finalize(s);
    check_error(r, SQLITE_OK);
}

void SqliteDatabase::createServiceTable()
{
    // create table and ignore errors
    auto q = "CREATE TABLE " + service_table + " (version TEXT NOT NULL DEFAULT \"\")";
    sqlite3_exec(db, q.c_str(), 0, 0, 0);

    int r = 0;

    // check rows count
    q = "SELECT count(*) FROM " + service_table;
    sqlite3_stmt *s;
    r = sqlite3_prepare_v2(db, q.c_str(), q.size(), &s, 0);
    check_error(r, SQLITE_OK);
    r = sqlite3_step(s);
    check_error(r, SQLITE_ROW);
    auto nrows = sqlite3_column_int64(s, 0);
    r = sqlite3_step(s);
    check_error(r, SQLITE_DONE);
    r = sqlite3_finalize(s);
    check_error(r, SQLITE_OK);

    if (nrows == 0)
    {
        // add main row
        q = "INSERT INTO " + service_table + " (version) values (-1)";
        sqlite3_stmt *s;
        r = sqlite3_prepare_v2(db, q.c_str(), q.size(), &s, 0);
        check_error(r, SQLITE_OK);
        r = sqlite3_step(s);
        check_error(r, SQLITE_DONE);
        r = sqlite3_finalize(s);
        check_error(r, SQLITE_OK);
    }
    else if (nrows > 1)
        throw std::runtime_error("db error: more than one row is present"s);
}

int SqliteDatabase::createServiceTableColumn(const String &col)
{
    auto q = "ALTER TABLE " + service_table + " ADD COLUMN " + col + " TEXT NOT NULL DEFAULT \"\"";
    return sqlite3_exec(db, q.c_str(), 0, 0, 0);
}

}
