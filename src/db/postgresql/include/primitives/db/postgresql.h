// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <primitives/db.h>

namespace primitives::db::postgresql
{

struct PostgresqlDatabase : primitives::db::LiteDatabase
{
    PostgresqlDatabase(const path &fn);
    PostgresqlDatabase(PostgresqlDatabase &&);
    virtual ~PostgresqlDatabase();

    void execute(const String &q) override;

private:
    static inline const String service_table{ "service.kv_settings" };

    void check_error(int r, int ec);

    optional<String> getValueByKey(const String &key) override;
    void setValueByKey(const String &key, const String &value) override;

    void createServiceTable();
    int createServiceTableColumn(const String &col);
};

}
