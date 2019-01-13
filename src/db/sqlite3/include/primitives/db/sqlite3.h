// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <primitives/db.h>

struct sqlite3;

namespace primitives::db::sqlite3
{

struct PRIMITIVES_DB_SQLITE3_API SqliteDatabase : primitives::db::LiteDatabase
{
    SqliteDatabase(const path &fn);
    SqliteDatabase(::sqlite3 *db);
    SqliteDatabase(SqliteDatabase &&);
    virtual ~SqliteDatabase();

    void execute(const String &q) override;

private:
    ::sqlite3 *db = nullptr;
    bool needs_close = true;
    static inline const String service_table{ "_svc_kv_settings" };

    void check_error(int r, int ec);

    std::optional<String> getValueByKey(const String &key) override;
    void setValueByKey(const String &key, const String &value) override;

    void createServiceTable();
    int createServiceTableColumn(const String &col);
};

}
