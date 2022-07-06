// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <primitives/db.h>
#include <primitives/exceptions.h>

#include <sqlite3.h>

namespace primitives::db::sqlite3
{

inline const auto service_table = "_svc_kv_settings"s;

struct SqliteDatabase : primitives::db::LiteDatabase
{
    SqliteDatabase(const path &fn)
    {
        sqlite3_open(fn.string().c_str(), &db);
    }
    SqliteDatabase(::sqlite3 *db)
        : db(db)
    {
        needs_close = false;
    }
    SqliteDatabase(SqliteDatabase &&rhs)
    {
        db = rhs.db;
        rhs.db = nullptr;
    }
    virtual ~SqliteDatabase()
    {
        if (needs_close)
            sqlite3_close(db);
    }

    void execute(const String &q) override
    {
        auto r = sqlite3_exec(db, q.c_str(), 0, 0, 0);
        check_error(r, SQLITE_OK);
    }

private:
    ::sqlite3 *db = nullptr;
    bool needs_close = true;

    void check_error(int r, int ec)
    {
        if (r == ec)
            return;
        auto err = sqlite3_errmsg(db);
        throw SW_RUNTIME_ERROR("db error: "s + err);
    }

    std::optional<String> getValueByKey(const String &key) override
    {
        createServiceTable();
        if (createServiceTableColumn(key) == SQLITE_OK)
            return {};

    #define CHECK_RC(rc, ec) if (r != ec) return {}

        int r = 0;
        auto q = "SELECT " + key + " FROM " + service_table;
        sqlite3_stmt *s;
        r = sqlite3_prepare_v2(db, q.c_str(), (int)q.size(), &s, 0);
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
    void setValueByKey(const String &key, const String &value) override
    {
        createServiceTable();
        createServiceTableColumn(key);

        int r = 0;
        auto q = "UPDATE " + service_table + " SET " + key + " = ?";
        sqlite3_stmt *s;
        r = sqlite3_prepare_v2(db, q.c_str(), (int)q.size(), &s, 0);
        check_error(r, SQLITE_OK);
        r = sqlite3_bind_text(s, 1, value.c_str(), -1, 0);
        check_error(r, SQLITE_OK);
        r = sqlite3_step(s);
        check_error(r, SQLITE_DONE);
        r = sqlite3_finalize(s);
        check_error(r, SQLITE_OK);
    }

    void createServiceTable()
    {
        // create table and ignore errors
        auto q = "CREATE TABLE " + service_table + " (version INTEGER NOT NULL DEFAULT \"\")";
        sqlite3_exec(db, q.c_str(), 0, 0, 0);

        int r = 0;

        // check rows count
        q = "SELECT count(*) FROM " + service_table;
        sqlite3_stmt *s;
        r = sqlite3_prepare_v2(db, q.c_str(), (int)q.size(), &s, 0);
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
            r = sqlite3_prepare_v2(db, q.c_str(), (int)q.size(), &s, 0);
            check_error(r, SQLITE_OK);
            r = sqlite3_step(s);
            check_error(r, SQLITE_DONE);
            r = sqlite3_finalize(s);
            check_error(r, SQLITE_OK);
        }
        else if (nrows > 1)
            throw SW_RUNTIME_ERROR("db error: more than one row is present"s);
    }
    int createServiceTableColumn(const String &col)
    {
        auto q = "ALTER TABLE " + service_table + " ADD COLUMN " + col + " TEXT NOT NULL DEFAULT \"\"";
        return sqlite3_exec(db, q.c_str(), 0, 0, 0);
    }
};

}
